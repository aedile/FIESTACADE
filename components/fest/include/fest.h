/*
 * fest.h - palette framebuffer and Fiesta decorations for the splash.
 *
 * The menu draws in RGB565 bands (see gfx.h) because it is mostly static and only
 * repaints what changed. The splash is the opposite: full-screen animation every
 * frame. Bands would mean re-reading and re-compositing the whole screen 30 times
 * a second, so the splash gets a real framebuffer instead - 8 bits per pixel,
 * 240x280 = 67 KB, half what RGB565 would cost, and cheap to fill and sprite into.
 *
 * It is allocated on entry and freed on exit, so the menu never pays for it.
 *
 * Colours are indices into a 6x6x5 RGB cube (0..179) plus a few named entries
 * above it. Palette entries are stored byte-swapped, ready for the panel.
 */
#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FB_W  240
#define FB_H  280

/* 6 reds x 6 greens x 5 blues = 180 entries, index 0..179 */
#define CUBE(r, g, b) ((uint8_t)((r) * 30 + (g) * 5 + (b)))

#define UI_BLACK  192
#define UI_WHITE  193
#define UI_GREY   194
#define UI_YELLOW 195
#define UI_GREEN  196
#define UI_RED    197
#define UI_BLUE   198

extern uint8_t *fest_fb;          /* FB_W * FB_H, NULL until fest_init() */
extern uint16_t fest_pal[256];

bool fest_init(void);             /* false if the framebuffer will not fit */
void fest_free(void);

void fest_clear(uint8_t colour);
void fest_px(int x, int y, uint8_t colour);
void fest_fill(int x, int y, int w, int h, uint8_t colour);
void fest_frame(int x, int y, int w, int h, uint8_t colour);
void fest_text(int x, int y, const char *s, uint8_t colour);
void fest_text_center(int y, const char *s, uint8_t colour);
void fest_text_scaled(int x, int y, const char *s, uint8_t colour, int scale);
void fest_present(void);          /* palette -> RGB565, pushed in strips */

/* decorations, all procedural - no artwork ships with them */
extern const uint8_t fiesta_colours[6];
void fest_papel_picado(int frame);
void fest_confetti(int frame);
void fest_dancers(int frame, int floor_y);

/* splash set pieces, also procedural */
void fest_stars(int frame);                            /* twinkling night sky */
void fest_skyline(void);                               /* city plus the Tower of the Americas */
void fest_beacon(int frame);                           /* red aircraft light on the spire */
void fest_fireworks(int frame);                        /* rockets that arc up and burst */
void fest_pinata(int frame, int cx, int top_y);        /* star pinata on a swinging cord */
void fest_cabinet(int frame, int cx, int base_y);      /* a lit-up upright arcade cabinet */

#ifdef __cplusplus
}
#endif
