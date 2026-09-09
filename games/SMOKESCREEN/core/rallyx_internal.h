#pragma once
#include "rallyx.h"
/* shared between the machine, the video and the sound */
extern rx_roms_t rx_roms;
extern uint8_t rx_vram[0x1000];      /* 0x8000-0x8FFF: tilemaps, sprites, radar dots */
extern uint8_t rx_radarattr[0x10];
extern uint8_t rx_scrollx, rx_scrolly, rx_flip;

void rx_wsg_init(const uint8_t *prom_wave);
void rx_wsg_reset(void);
void rx_wsg_write(int reg, uint8_t data);
void rx_wsg_render(int16_t *buf, int samples, int sample_rate);
void rx_bang(int state);
void rx_bang_render(int16_t *buf, int samples, int sample_rate);
