#ifndef CENTIPED_INTERNAL_H
#define CENTIPED_INTERNAL_H
#include "centiped.h"

extern uint8_t ce_ram_bytes[0x800];          /* 0x0400-0x07BF playfield, 0x07C0-0x07FF sprites */

void ce_video_init(const ce_roms_t *r);
void ce_video_reset(void);
void ce_video_palette_w(int offset, uint8_t data);
void ce_video_render(uint8_t *fb);

#endif
