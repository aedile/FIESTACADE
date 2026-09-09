#pragma once
#include "joust.h"
extern jo_roms_t jo_roms;
extern uint8_t jo_vram[0xc000];
extern uint8_t jo_palram[16];

/* the sound board */
void jo_sound_init(void);
void jo_sound_reset(void);
void jo_sound_cmd(uint8_t d);          /* the main board's PIA writes the command lines */
void jo_sound_run(int32_t cycles);
