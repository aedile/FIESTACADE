/*
 * render.cpp - put Joust's 292x240 picture on the 240x280 panel.
 *
 * The cabinet's monitor is the normal way up, so the picture is landscape and the panel is
 * portrait: 240 rows of it go on at one to one with a 20-row bar above and below, and the
 * middle 280 of its 292 columns are squeezed to 240 by six to seven. Rather than drop one
 * column in seven - which takes whole strokes out of the lettering - the fourth and fifth of
 * every seven are averaged, through a table of blended palette pairs.
 *
 * The frame arrives as the hardware keeps it, two pixels a byte running down each column,
 * so a panel row is a walk across 146 column buffers.
 */
#include "render.h"
#include "joust.h"
#include "display.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "RENDER";
#define ROWS_PER_CHUNK 14
#define NUM_FB 2
#define PIC_H JO_SCREEN_H                              /* 240 */
#define TOP_BAR ((DISPLAY_HEIGHT - PIC_H) / 2)         /* 20 */
#define LEFT_TRIM ((JO_SCREEN_W - 280) / 2)            /* 6: the columns that never show */

typedef struct { uint8_t *snap; uint16_t pal[JO_PALETTE_SIZE]; } frame_t;
static frame_t frames[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t *chunk;
static uint16_t pal_swapped[JO_PALETTE_SIZE];
static uint16_t blend_swapped[JO_PALETTE_SIZE * JO_PALETTE_SIZE];   /* indexed by (left << 4) | right */
static uint16_t last_pal[JO_PALETTE_SIZE];
static bool have_tables;
static uint16_t col_ofs[DISPLAY_WIDTH];                /* panel column -> its column buffer's offset */
static uint8_t col_shift[DISPLAY_WIDTH];               /* 4 for an even visible column, 0 for an odd */
static uint8_t col_blend[DISPLAY_WIDTH];               /* 0 plain, 1 both pixels in this byte, 2 straddles the next column */
static uint32_t frames_drawn, frames_dropped;
static uint64_t busy_us;

static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v >> 8) | (v << 8)); }

static void build_tables(const uint16_t *pal)
{
    for (int i = 0; i < JO_PALETTE_SIZE; i++) pal_swapped[i] = swap16(pal[i]);
    for (int a = 0; a < JO_PALETTE_SIZE; a++)
        for (int b = 0; b < JO_PALETTE_SIZE; b++) {
            uint16_t pa = pal[a], pb = pal[b];
            int r = (((pa >> 11) & 0x1f) + ((pb >> 11) & 0x1f)) >> 1;
            int g = (((pa >> 5) & 0x3f) + ((pb >> 5) & 0x3f)) >> 1;
            int bl = ((pa & 0x1f) + (pb & 0x1f)) >> 1;
            blend_swapped[(a << 4) | b] = swap16((uint16_t)((r << 11) | (g << 5) | bl));
        }
    memcpy(last_pal, pal, sizeof(last_pal));
    have_tables = true;
}

/*
 * A chunk of rows, column by column: each panel column is a run of consecutive bytes in one
 * column buffer, so the inner loop is a load, a shift, a table lookup and a strided store.
 */
static void convert_rows(const uint8_t *snap, int y0, int rows, uint16_t *out)
{
    for (int px = 0; px < DISPLAY_WIDTH; px++) {
        const uint8_t *col = snap + col_ofs[px] + y0;
        uint16_t *d = out + px;
        int sh = col_shift[px];
        switch (col_blend[px]) {
            case 0:
                for (int r = 0; r < rows; r++, d += DISPLAY_WIDTH) *d = pal_swapped[(col[r] >> sh) & 0x0f];
                break;
            case 1:                                        /* even column: this byte holds both */
                for (int r = 0; r < rows; r++, d += DISPLAY_WIDTH) *d = blend_swapped[col[r]];
                break;
            default: {                                     /* odd column: low nibble here, high nibble next door */
                const uint8_t *nxt = col + JO_SCREEN_H;
                for (int r = 0; r < rows; r++, d += DISPLAY_WIDTH) *d = blend_swapped[((col[r] & 0x0f) << 4) | (nxt[r] >> 4)];
                break;
            }
        }
    }
}

static void present(const frame_t *f)
{
    if (!have_tables || memcmp(last_pal, f->pal, sizeof(last_pal)) != 0) build_tables(f->pal);
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int row = 0; row < DISPLAY_HEIGHT; row += ROWS_PER_CHUNK) {
        int rows = (row + ROWS_PER_CHUNK <= DISPLAY_HEIGHT) ? ROWS_PER_CHUNK : (DISPLAY_HEIGHT - row);
        int y0 = row - TOP_BAR, y1 = y0 + rows;            /* picture rows covered by this chunk */
        if (y1 <= 0 || y0 >= PIC_H) {
            memset(chunk, 0, rows * DISPLAY_WIDTH * sizeof(uint16_t));
        } else if (y0 >= 0 && y1 <= PIC_H) {
            convert_rows(f->snap, y0, rows, chunk);
        } else {                                           /* a chunk that straddles a bar */
            memset(chunk, 0, rows * DISPLAY_WIDTH * sizeof(uint16_t));
            int a = y0 < 0 ? 0 : y0, b = y1 > PIC_H ? PIC_H : y1;
            convert_rows(f->snap, a, b - a, chunk + (a - y0) * DISPLAY_WIDTH);
        }
        display_write_preswapped(chunk, rows * DISPLAY_WIDTH);
    }
    display_wait_done();
}

static void render_task(void *arg)
{
    (void)arg;
    for (;;) {
        frame_t *f;
        if (xQueueReceive(frame_q, &f, portMAX_DELAY) != pdTRUE) continue;
        int64_t t0 = esp_timer_get_time();
        present(f);
        busy_us += esp_timer_get_time() - t0;
        xQueueSend(free_q, &f, 0);
        frames_drawn++;
    }
}

void render_init(void)
{
    /* seven visible columns become six panel columns: 0 1 2 (3+4) 5 6 */
    for (int px = 0; px < DISPLAY_WIDTH; px++) {
        int group = px / 6, k = px % 6;
        int x = LEFT_TRIM + group * 7 + (k < 3 ? k : k + 1);
        col_ofs[px] = (uint16_t)((x >> 1) * JO_SCREEN_H);
        col_shift[px] = (x & 1) ? 0 : 4;
        col_blend[px] = (k != 3) ? 0 : ((x & 1) ? 2 : 1);
    }
    chunk = (uint16_t *)heap_caps_malloc(ROWS_PER_CHUNK * DISPLAY_WIDTH * sizeof(uint16_t), MALLOC_CAP_8BIT);
    free_q = xQueueCreate(NUM_FB, sizeof(frame_t *));
    frame_q = xQueueCreate(NUM_FB, sizeof(frame_t *));
    for (int i = 0; i < NUM_FB; i++) {
        frames[i].snap = (uint8_t *)heap_caps_malloc(JO_SNAP_SIZE, MALLOC_CAP_8BIT);
        if (!frames[i].snap) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        frame_t *f = &frames[i];
        xQueueSend(free_q, &f, 0);
    }
    if (!chunk) { ESP_LOGE(TAG, "chunk allocation failed"); abort(); }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started (%dx%d, squeezed 7:6, %d-row bars)", JO_SCREEN_W, JO_SCREEN_H, TOP_BAR);
}

uint8_t *render_acquire(void)
{
    frame_t *f;
    if (xQueueReceive(free_q, &f, 0) != pdTRUE) { frames_dropped++; return nullptr; }
    return f->snap;
}

void render_submit(uint8_t *snap)
{
    for (int i = 0; i < NUM_FB; i++)
        if (frames[i].snap == snap) { frame_t *f = &frames[i]; jo_palette(f->pal); xQueueSend(frame_q, &f, 0); return; }
}
uint32_t render_frames_drawn(void) { uint32_t v = frames_drawn; frames_drawn = 0; return v; }
uint32_t render_frames_dropped(void) { uint32_t v = frames_dropped; frames_dropped = 0; return v; }
uint64_t render_busy_us(void) { uint64_t v = busy_us; busy_us = 0; return v; }
