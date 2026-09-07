#pragma once
#include "stubs.h"
#define DISPLAY_WIDTH 240
#define DISPLAY_HEIGHT 280
#ifdef __cplusplus
extern "C" {
#endif
void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void display_write_preswapped(const uint16_t *px, uint32_t n);
void display_wait_done(void);
#ifdef __cplusplus
}
#endif
