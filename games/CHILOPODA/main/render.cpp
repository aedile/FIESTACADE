/*
 * render.cpp - turn the 256x240 native frame upright onto the 240x280 panel from a task.
 * Panel (px 0..239, py 0..279): native x = 255 - (py - 12), native y = px; 12 blank rows top and bottom.
 */
#include "render.h"
#include "centiped.h"
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
#define Y_MARGIN ((DISPLAY_HEIGHT - CE_FB_W) / 2)   /* 12 */
#define FLIP 0                                      /* 1 if the picture comes out upside down */

static uint8_t *fbs[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t *chunk;
static uint16_t pal_swapped[CE_PALETTE_SIZE];
static uint32_t frames_drawn, frames_dropped;
static uint64_t busy_us;

static void present(const uint8_t *fb)
{
    uint16_t pal[CE_PALETTE_SIZE];
    ce_palette(pal);
    for (int i = 0; i < CE_PALETTE_SIZE; i++) pal_swapped[i] = (uint16_t)((pal[i] >> 8) | (pal[i] << 8));
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int row = 0; row < DISPLAY_HEIGHT; row += ROWS_PER_CHUNK) {
        int rows = (row + ROWS_PER_CHUNK <= DISPLAY_HEIGHT) ? ROWS_PER_CHUNK : (DISPLAY_HEIGHT - row);
        uint16_t *dst = chunk;
        for (int r = 0; r < rows; r++) {
            int py = row + r;
            int nx = py - Y_MARGIN;
            if (nx < 0 || nx >= CE_FB_W) { for (int px = 0; px < DISPLAY_WIDTH; px++) *dst++ = 0; continue; }
#if FLIP
            const uint8_t *src = fb + (CE_FB_H - 1) * CE_FB_W + nx;          /* native y runs 239..0 */
            for (int px = 0; px < DISPLAY_WIDTH; px++) { *dst++ = pal_swapped[*src]; src -= CE_FB_W; }
#else
            const uint8_t *src = fb + (CE_FB_W - 1 - nx);                    /* native x = 255 - nx, y runs 0..239 */
            for (int px = 0; px < DISPLAY_WIDTH; px++) { *dst++ = pal_swapped[*src]; src += CE_FB_W; }
#endif
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
    chunk = (uint16_t *)heap_caps_malloc(ROWS_PER_CHUNK * DISPLAY_WIDTH * sizeof(uint16_t), MALLOC_CAP_8BIT);
    free_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    frame_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    for (int i = 0; i < NUM_FB; i++) {
        fbs[i] = (uint8_t *)heap_caps_malloc(CE_FB_W * CE_FB_H, MALLOC_CAP_8BIT);
        if (!fbs[i]) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        xQueueSend(free_q, &fbs[i], 0);
    }
    if (!chunk) { ESP_LOGE(TAG, "chunk allocation failed"); abort(); }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started");
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
