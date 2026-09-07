/*
 * gfx.h - band compositor and stroke font for the MINIMAME launcher.
 *
 * There is no framebuffer. A full 240x280 RGB565 buffer would be 131 KB of the
 * C6's 512 KB with no PSRAM, so the screen is drawn as horizontal bands: clear a
 * band, draw whatever intersects it, push it, repeat. One band buffer is all the
 * RAM the UI needs.
 *
 * IMPORTANT: every pixel in a band is RGB565 stored BIG-ENDIAN, matching the
 * marquee blob and display_write_preswapped(). Build colours with gfx_rgb(),
 * which returns a value already in that order - do not write raw RGB565.
 */
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_W       240
#define GFX_H       280
#define GFX_BAND_H  40                      /* 240*40*2 = 19200 bytes */

#define GFX_GLYPH_H 10                      /* text cell height, before scaling */
#define GFX_ADVANCE 8

typedef struct {
    uint16_t *px;                           /* GFX_W * GFX_BAND_H, big-endian RGB565 */
    int       y0;                           /* screen row of the band's first line */
    int       h;                            /* rows valid in this pass */
} gfx_band_t;

/* Pack to RGB565 and byte-swap in one step. */
static inline uint16_t gfx_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t c = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    return (uint16_t)((c >> 8) | (c << 8));
}

void gfx_clear(gfx_band_t *b, uint16_t colour);
void gfx_pixel(gfx_band_t *b, int x, int y, uint16_t colour);
void gfx_fill_rect(gfx_band_t *b, int x, int y, int w, int h, uint16_t colour);
void gfx_line(gfx_band_t *b, int x0, int y0, int x1, int y1, uint16_t colour);

int  gfx_text_width(const char *s, int scale);
void gfx_text(gfx_band_t *b, int x, int y, const char *s, int scale, uint16_t colour);
void gfx_text_center(gfx_band_t *b, int cx, int y, const char *s, int scale, uint16_t colour);

#ifdef __cplusplus
}
#endif
