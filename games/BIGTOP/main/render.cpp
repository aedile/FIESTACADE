/*
 * render.cpp - put Mr. Do!'s 240x192 picture on the 240x280 panel.
 *
 * The cabinet's monitor is turned a quarter turn anticlockwise (MAME's ROT270): the
 * hardware's top edge is the player's left. So a frame row becomes a panel column and a frame
 * column becomes a panel row, read from the last column to the first. The 192x240 picture
 * that makes is scaled by 7/6 to 224x280, filling the panel's height with an 8-column bar
 * each side.
 */
#include "render.h"
#include "mrdo.h"
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
#define PIC_W (MD_FB_H * DISPLAY_HEIGHT / MD_FB_W)   /* 224 */
#define PIC_H DISPLAY_HEIGHT                         /* 280 */
#define SIDE_BAR ((DISPLAY_WIDTH - PIC_W) / 2)       /* 8 */

static uint8_t *fbs[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t *chunk;
static uint16_t pal_swapped[MD_PALETTE_SIZE];
static uint16_t col_map[DISPLAY_WIDTH];            /* panel column -> frame row, or 0xffff for a bar */
static uint16_t row_map[DISPLAY_HEIGHT];           /* panel row    -> frame column */
static uint32_t frames_drawn, frames_dropped;
static uint64_t busy_us;

static void present(const uint8_t *fb)
{
    uint16_t pal[MD_PALETTE_SIZE];
    md_palette(pal);
    for (int i = 0; i < MD_PALETTE_SIZE; i++) pal_swapped[i] = (uint16_t)((pal[i] >> 8) | (pal[i] << 8));
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int row = 0; row < DISPLAY_HEIGHT; row += ROWS_PER_CHUNK) {
        int rows = (row + ROWS_PER_CHUNK <= DISPLAY_HEIGHT) ? ROWS_PER_CHUNK : (DISPLAY_HEIGHT - row);
        uint16_t *dst = chunk;
        for (int r = 0; r < rows; r++) {
            int fx = row_map[row + r];
            for (int px = 0; px < DISPLAY_WIDTH; px++) {
                uint16_t fy = col_map[px];
                *dst++ = (fy == 0xffff) ? 0 : pal_swapped[fb[fy * MD_FB_W + fx]];
            }
        }
        display_write_preswapped(chunk, rows * DISPLAY_WIDTH);
    }
    display_wait_done();
}

static void render_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint8_t *fb;
        if (xQueueReceive(frame_q, &fb, portMAX_DELAY) != pdTRUE) continue;
        int64_t t0 = esp_timer_get_time();
        present(fb);
        busy_us += esp_timer_get_time() - t0;
        xQueueSend(free_q, &fb, 0);
        frames_drawn++;
    }
}

void render_init(void)
{
    /* a quarter turn anticlockwise: the frame's top edge becomes the panel's left edge, so
     * panel column c is frame row c' and panel row r is frame column 239 - r' */
    for (int i = 0; i < DISPLAY_WIDTH; i++) {
        int p = i - SIDE_BAR;
        col_map[i] = (p < 0 || p >= PIC_W) ? 0xffff : (uint16_t)(p * MD_FB_H / PIC_W);
    }
    for (int i = 0; i < DISPLAY_HEIGHT; i++) row_map[i] = (uint16_t)(MD_FB_W - 1 - i * MD_FB_W / PIC_H);
    chunk = (uint16_t *)heap_caps_malloc(ROWS_PER_CHUNK * DISPLAY_WIDTH * sizeof(uint16_t), MALLOC_CAP_8BIT);
    free_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    frame_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    for (int i = 0; i < NUM_FB; i++) {
        fbs[i] = (uint8_t *)heap_caps_malloc(MD_FB_W * MD_FB_H, MALLOC_CAP_8BIT);
        if (!fbs[i]) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        xQueueSend(free_q, &fbs[i], 0);
    }
    if (!chunk) { ESP_LOGE(TAG, "chunk allocation failed"); abort(); }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started (%dx%d native, %dx%d picture, %d-column bars)", MD_FB_W, MD_FB_H, PIC_W, PIC_H, SIDE_BAR);
}

uint8_t *render_acquire(void)
{
    uint8_t *fb;
    if (xQueueReceive(free_q, &fb, 0) != pdTRUE) { frames_dropped++; return nullptr; }
    return fb;
}
void render_submit(uint8_t *fb) { xQueueSend(frame_q, &fb, 0); }
uint32_t render_frames_drawn(void) { uint32_t v = frames_drawn; frames_drawn = 0; return v; }
uint32_t render_frames_dropped(void) { uint32_t v = frames_dropped; frames_dropped = 0; return v; }
uint64_t render_busy_us(void) { uint64_t v = busy_us; busy_us = 0; return v; }
