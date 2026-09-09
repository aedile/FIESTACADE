/*
 * render.cpp - put Moon Patrol's 240x252 picture on the 240x280 panel.
 *
 * The cabinet's monitor is the normal way up and the picture is exactly the panel's width, so
 * it goes on at one to one with a 14-row bar above and below.
 */
#include "render.h"
#include "mpatrol.h"
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
#define ROWS_PER_CHUNK 28
#define NUM_FB 2
#define TOP_BAR ((DISPLAY_HEIGHT - MP_FB_H) / 2)     /* 14 */

static uint8_t *fbs[NUM_FB];
static QueueHandle_t free_q, frame_q;
static uint16_t pal_swapped[MP_PALETTE_SIZE];
static uint32_t frames_drawn, frames_dropped;
static uint64_t busy_us;

static void present(const uint8_t *fb)
{
    display_set_window(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    for (int row = 0; row < DISPLAY_HEIGHT; row += ROWS_PER_CHUNK) {
        int rows = (row + ROWS_PER_CHUNK <= DISPLAY_HEIGHT) ? ROWS_PER_CHUNK : (DISPLAY_HEIGHT - row);
        uint16_t *dst = display_acquire_buffer();
        for (int r = 0; r < rows; r++) {
            int y = row + r - TOP_BAR;
            if (y < 0 || y >= MP_FB_H) { memset(dst, 0, DISPLAY_WIDTH * sizeof(uint16_t)); dst += DISPLAY_WIDTH; continue; }
            const uint8_t *src = fb + y * MP_FB_W;
            const uint16_t *pal = pal_swapped;
            for (int px = 0; px < DISPLAY_WIDTH; px += 8, src += 8, dst += 8) {
                uint8_t a = src[0], b = src[1], c = src[2], d = src[3], e = src[4], f = src[5], g = src[6], h = src[7];
                dst[0] = pal[a]; dst[1] = pal[b]; dst[2] = pal[c]; dst[3] = pal[d];
                dst[4] = pal[e]; dst[5] = pal[f]; dst[6] = pal[g]; dst[7] = pal[h];
            }
        }
        display_submit_buffer(rows * DISPLAY_WIDTH);
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
    uint16_t pal[MP_PALETTE_SIZE];
    mp_palette(pal);                                   /* fixed: it comes from the PROMs */
    for (int i = 0; i < MP_PALETTE_SIZE; i++) pal_swapped[i] = (uint16_t)((pal[i] >> 8) | (pal[i] << 8));
    free_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    frame_q = xQueueCreate(NUM_FB, sizeof(uint8_t *));
    for (int i = 0; i < NUM_FB; i++) {
        fbs[i] = (uint8_t *)heap_caps_malloc(MP_FB_W * MP_FB_H, MALLOC_CAP_8BIT);
        if (!fbs[i]) { ESP_LOGE(TAG, "frame buffer allocation failed"); abort(); }
        xQueueSend(free_q, &fbs[i], 0);
    }
    xTaskCreate(render_task, "render", 4096, nullptr, 6, nullptr);
    ESP_LOGI(TAG, "render task started (%dx%d native, %d-row bars)", MP_FB_W, MP_FB_H, TOP_BAR);
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
