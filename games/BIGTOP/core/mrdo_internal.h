#pragma once
#include "mrdo.h"
extern md_roms_t md_roms;
extern uint8_t md_bgram[0x800], md_fgram[0x800], md_sprram[0x100];
extern uint8_t md_flip, md_scrollx, md_scrolly;
void md_video_init(void);
