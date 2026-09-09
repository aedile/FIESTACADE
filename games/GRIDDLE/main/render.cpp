/*
 * render.cpp - put Burger Time's 240x240 picture on the 240x280 panel.
 *
 * The cabinet's monitor is turned a quarter turn anticlockwise (MAME's ROT270), so a frame
 * column becomes a panel row and a frame row a panel column. The picture is square and the
 * panel's width, so it goes on at one to one with a 20-row bar above and below.
 */
#include "render.h"
#include "btime.h"
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
#define TOP_BAR ((DISPLAY_HEIGHT - BT_FB_W) / 2)     /* 20 */

static uint8_t *fbs[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t *chunk;
static uint16_t pal_swapped[BT_PALETTE_SIZE];
static uint32_t frames_drawn, frames_dropped;
static uint64_t busy_us;

static void present(const uint8_t *fb)
{
    uint16_t pal[BT_PALETTE_SIZE];
    bt_palette(pal);
    for (int i = 0; i < BT_PALETTE_SIZE; i++) pal_swapped[i] = (uint16_t)((pal[i] >> 8) | (pal[i] << 8));
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int row = 0; row < DISPLAY_HEIGHT; row += ROWS_PER_CHUNK) {
        int rows = (row + ROWS_PER_CHUNK <= DISPLAY_HEIGHT) ? ROWS_PER_CHUNK : (DISPLAY_HEIGHT - row);
        uint16_t *dst = chunk;
        for (int r = 0; r < rows; r++) {
            int p = row + r - TOP_BAR;
            if (p < 0 || p >= BT_FB_W) { memset(dst, 0, DISPLAY_WIDTH * sizeof(uint16_t)); dst += DISPLAY_WIDTH; continue; }
            /* panel row p is frame column 239 - p; panel column c is frame row c */
            int fx = BT_FB_W - 1 - p;
            const uint8_t *src = fb + fx;
            for (int px = 0; px < DISPLAY_WIDTH; px++, src += BT_FB_W) *dst++ = pal_swapped[*src];
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
        fbs[i] = (uint8_t *)heap_caps_malloc(BT_FB_W * BT_FB_H, MALLOC_CAP_8BIT);
        if (!fbs[i]) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        xQueueSend(free_q, &fbs[i], 0);
    }
    if (!chunk) { ESP_LOGE(TAG, "chunk allocation failed"); abort(); }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started (%dx%d native, turned, %d-row bars)", BT_FB_W, BT_FB_H, TOP_BAR);
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
