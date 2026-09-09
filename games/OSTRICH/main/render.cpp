/*
 * render.cpp - put Joust's 292x240 picture on the 240x280 panel.
 *
 * The cabinet's monitor is the normal way up, so the picture is landscape and the panel is
 * portrait: 240 rows of it go on at one to one with a 20-row bar above and below, and the
 * middle 280 of its 292 columns are squeezed to 240 by six to seven. Dropping a fixed column
 * in seven takes whole strokes out of the lettering, and averaging a fixed pair leaves them
 * half-bright, which reads the same. So the choice is made per row and per group of seven:
 * the first adjacent pair of equal pixels, looked for from the middle outwards, loses one of
 * the two - nothing is lost, there is always such a pair in text and in most of the artwork -
 * and only a group with no equal pair at all has its middle pair averaged.
 *
 * The frame arrives as the hardware keeps it, two pixels a byte running down each column.
 */
#include "render.h"
#include "joust.h"
#include "display.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "RENDER";
#define ROWS_PER_CHUNK 28
#define NUM_FB 2
#define PIC_H JO_SCREEN_H                              /* 240 */
#define TOP_BAR ((DISPLAY_HEIGHT - PIC_H) / 2)         /* 20 */
#define LEFT_TRIM ((JO_SCREEN_W - 280) / 2)            /* 6: the columns that never show */

typedef struct { uint8_t *snap; uint16_t pal[JO_PALETTE_SIZE]; } frame_t;
static frame_t frames[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t pal_swapped[JO_PALETTE_SIZE];
static uint16_t blend_swapped[JO_PALETTE_SIZE * JO_PALETTE_SIZE];   /* indexed by (left << 4) | right */
static uint16_t last_pal[JO_PALETTE_SIZE];
static bool have_tables;
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
 * A chunk of rows, one group of seven visible columns at a time: the group's four column
 * buffers are read as consecutive runs, each row's eight pens packed into a word, and the
 * equal pair found by nibble arithmetic rather than six compares.
 */
static uint8_t drop_choice[64];                        /* six equal-pair flags -> pen to drop, or 7 */
static uint16_t group_col[40];                         /* the group's first column buffer, in bytes */
static uint8_t group_odd[40];                          /* 1: its first pixel is a low nibble */

static void IRAM_ATTR convert_rows(const uint8_t *snap, int y0, int rows, uint16_t *out)
{
    for (int g = 0; g < 40; g++) {
        const uint8_t *c0 = snap + group_col[g] + y0, *c1 = c0 + JO_SCREEN_H, *c2 = c1 + JO_SCREEN_H, *c3 = c2 + JO_SCREEN_H;
        int odd = group_odd[g];
        uint16_t *d = out + g * 6;
        for (int r = 0; r < rows; r++, d += DISPLAY_WIDTH) {
            uint32_t w = ((uint32_t)c0[r] << 24) | ((uint32_t)c1[r] << 16) | ((uint32_t)c2[r] << 8) | c3[r];
            if (odd) w <<= 4;                          /* pen 0 at the top nibble either way */
            /* a zero nibble in x marks a pair of equal neighbours; gather the six flags */
            uint32_t x = w ^ (w << 4);
            uint32_t t = x | (x >> 1); t |= t >> 2;
            uint32_t z = ~t & 0x11111111u;
            unsigned m = ((z >> 28) & 1) | ((z >> 23) & 2) | ((z >> 18) & 4) | ((z >> 13) & 8) | ((z >> 8) & 16) | ((z >> 3) & 32);
            int j = drop_choice[m];
            if (j == 7) {
                d[0] = pal_swapped[w >> 28]; d[1] = pal_swapped[(w >> 24) & 15]; d[2] = pal_swapped[(w >> 20) & 15];
                d[3] = blend_swapped[(w >> 12) & 0xff];
                d[4] = pal_swapped[(w >> 8) & 15]; d[5] = pal_swapped[(w >> 4) & 15];
            } else {
                /* close the gap: keep the pens above j, pull the ones below up a nibble */
                uint32_t keep = 0xffffffffu << (32 - 4 * j);
                if (j == 0) keep = 0;
                uint32_t v = (w & keep) | ((w << 4) & ~keep);
                d[0] = pal_swapped[v >> 28]; d[1] = pal_swapped[(v >> 24) & 15]; d[2] = pal_swapped[(v >> 20) & 15];
                d[3] = pal_swapped[(v >> 16) & 15]; d[4] = pal_swapped[(v >> 12) & 15]; d[5] = pal_swapped[(v >> 8) & 15];
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
        uint16_t *chunk = display_acquire_buffer();
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
        display_submit_buffer(rows * DISPLAY_WIDTH);
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
    /* which pen to drop for each pattern of equal neighbours: the middle pairs first */
    static const uint8_t order[6] = { 3, 2, 4, 1, 5, 0 };
    for (int m = 0; m < 64; m++) {
        drop_choice[m] = 7;
        for (int k = 0; k < 6; k++) if (m & (1 << order[k])) { drop_choice[m] = order[k]; break; }
    }
    for (int g = 0; g < 40; g++) {
        int x0 = LEFT_TRIM + g * 7;
        group_col[g] = (uint16_t)((x0 >> 1) * JO_SCREEN_H);
        group_odd[g] = (uint8_t)(x0 & 1);
    }
    free_q = xQueueCreate(NUM_FB, sizeof(frame_t *));
    frame_q = xQueueCreate(NUM_FB, sizeof(frame_t *));
    for (int i = 0; i < NUM_FB; i++) {
        frames[i].snap = (uint8_t *)heap_caps_malloc(JO_SNAP_SIZE, MALLOC_CAP_8BIT);
        if (!frames[i].snap) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        frame_t *f = &frames[i];
        xQueueSend(free_q, &f, 0);
    }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started (%dx%d, squeezed 7:6 adaptively, %d-row bars)", JO_SCREEN_W, JO_SCREEN_H, TOP_BAR);
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
