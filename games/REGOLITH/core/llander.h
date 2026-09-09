/*
 * llander.h - Atari Lunar Lander (1979) board emulation
 *
 * Asteroids' board a few months earlier: the same 6502 at 1.512 MHz and the same Digital
 * Vector Generator, with 8 KB of program, 6 KB of vector ROM, an analogue throttle read
 * through an ADC, and a different set of sound latches. There is no frame buffer: the CPU
 * builds a display list in vector RAM and the DVG walks it, so what comes out of this core is
 * a list of line segments.
 *
 * The sound is discrete: thrust is noise whose level follows a three-bit DAC, the explosion
 * is noise through a filter, and the two beeps are a 3 kHz and a 6 kHz tone. Those are
 * synthesised here from the same latches.
 *
 * Timing and memory map follow MAME's asteroid.cpp (the llander map).
 */
#ifndef LLANDER_H
#define LLANDER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define LL_MASTER_CLOCK  12096000
#define LL_CPU_CLOCK     (LL_MASTER_CLOCK / 8)          /* 1.512 MHz */
/* The periodic NMI runs at 12.096 MHz / 4096 / 12 = 246.09 Hz, and the display is refreshed
 * on every fourth one, so a frame is 61.523 Hz and exactly 24576 CPU cycles. */
#define LL_NMI_CYCLES    6144
#define LL_NMIS_PER_FRAME 4
#define LL_CYCLES_PER_FRAME (LL_NMI_CYCLES * LL_NMIS_PER_FRAME)

/* The DVG's position counters are 12 bits; y increases upwards. Lunar Lander's picture is
 * wider than Asteroids': the beam runs 0..1280 across and the game keeps everything between
 * 0 and 800 vertically (the readouts sit at 768, the terrain floor near 24). */
#define LL_X_MIN 0
#define LL_X_MAX 1280
#define LL_Y_MIN 0
#define LL_Y_MAX 800
#define LL_MAX_LINES 900

typedef struct {
    int16_t x0, y0, x1, y1;
    uint8_t bright;              /* 1..15; segments with intensity 0 are not emitted */
} ll_line_t;

typedef struct {
    const uint8_t *rom;          /* 8 KB program, CPU 0x6000-0x7FFF */
    const uint8_t *vecrom;       /* 6 KB vector ROM, CPU 0x4800-0x5FFF */
} ll_roms_t;

typedef struct {
    uint8_t left, right, abort_, select;   /* select is the "Select Game" button */
    uint8_t thrust;                         /* the throttle, 0..255 */
    uint8_t start1, coin1;
} ll_input_t;

void ll_init(const ll_roms_t *roms);
void ll_reset(void);
void ll_set_dips(uint8_t dsw1);           /* see the DIP table in llander.c */
ll_input_t *ll_input(void);

void ll_run_frame(void);
/* the most recently completed display list */
const ll_line_t *ll_lines(int *count);
void ll_render_audio(int16_t *buf, int samples, int rate);

/* diagnostics */
uint16_t ll_pc(void);
uint32_t ll_frame_count(void);
const uint8_t *ll_ram(void);
extern uint32_t ll_dbg_snd_writes[256];

#ifdef __cplusplus
}
#endif
#endif
