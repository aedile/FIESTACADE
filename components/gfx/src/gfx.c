/*
 * gfx.c - band compositor and stroke font.
 *
 * The glyph set is carried over from TRENCHRUNNER's marquee.cpp so lettering
 * matches across the medals: each glyph is a set of polylines on a 6-wide,
 * 10-tall grid with y pointing up, strokes separated by ';' and points by spaces.
 */
#include "gfx.h"
#include <string.h>

static const char *const glyphs[] = {
    /* A */ "0,0 0,7 3,10 6,7 6,0;0,4 6,4",
    /* B */ "0,0 0,10 4,10 6,8 6,6 4,5 0,5;4,5 6,4 6,2 4,0 0,0",
    /* C */ "6,8 4,10 2,10 0,8 0,2 2,0 4,0 6,2",
    /* D */ "0,0 0,10 4,10 6,8 6,2 4,0 0,0",
    /* E */ "6,0 0,0 0,10 6,10;0,5 4,5",
    /* F */ "0,0 0,10 6,10;0,5 4,5",
    /* G */ "6,8 4,10 2,10 0,8 0,2 2,0 4,0 6,2 6,4 3,4",
    /* H */ "0,0 0,10;6,0 6,10;0,5 6,5",
    /* I */ "1,0 5,0;3,0 3,10;1,10 5,10",
    /* J */ "0,2 2,0 4,0 6,2 6,10",
    /* K */ "0,0 0,10;6,10 0,4;2,6 6,0",
    /* L */ "0,10 0,0 6,0",
    /* M */ "0,0 0,10 3,6 6,10 6,0",
    /* N */ "0,0 0,10 6,0 6,10",
    /* O */ "0,2 0,8 2,10 4,10 6,8 6,2 4,0 2,0 0,2",
    /* P */ "0,0 0,10 4,10 6,8 6,6 4,4 0,4",
    /* Q */ "0,2 0,8 2,10 4,10 6,8 6,2 4,0 2,0 0,2;3,3 6,0",
    /* R */ "0,0 0,10 4,10 6,8 6,6 4,4 0,4;3,4 6,0",
    /* S */ "6,8 4,10 2,10 0,8 0,6 2,5 4,5 6,4 6,2 4,0 2,0 0,2",
    /* T */ "0,10 6,10;3,10 3,0",
    /* U */ "0,10 0,2 2,0 4,0 6,2 6,10",
    /* V */ "0,10 3,0 6,10",
    /* W */ "0,10 0,0 3,4 6,0 6,10",
    /* X */ "0,0 6,10;0,10 6,0",
    /* Y */ "0,10 3,5 6,10;3,5 3,0",
    /* Z */ "0,10 6,10 0,0 6,0",
    /* 0 */ "0,2 0,8 2,10 4,10 6,8 6,2 4,0 2,0 0,2",
    /* 1 */ "1,8 3,10 3,0;1,0 5,0",
    /* 2 */ "0,8 2,10 4,10 6,8 6,6 0,0 6,0",
    /* 3 */ "0,10 6,10 3,6 6,4 6,2 4,0 2,0 0,2",
    /* 4 */ "4,0 4,10 0,4 6,4",
    /* 5 */ "6,10 0,10 0,5 4,5 6,3 6,2 4,0 0,0",
    /* 6 */ "6,8 4,10 2,10 0,8 0,2 2,0 4,0 6,2 6,4 4,5 0,5",
    /* 7 */ "0,10 6,10 2,0",
    /* 8 */ "2,5 0,7 0,8 2,10 4,10 6,8 6,7 4,5 2,5 0,3 0,2 2,0 4,0 6,2 6,3 4,5",
    /* 9 */ "0,2 2,0 4,0 6,2 6,8 4,10 2,10 0,8 0,6 2,5 6,5",
    /* - */ "1,5 5,5",
    /* . */ "2,0 3,0 3,1 2,1 2,0",
    /* , */ "3,1 2,-1",
    /* : */ "3,7 3,8;3,2 3,3",
    /* ! */ "3,10 3,3;3,0 3,1",
    /* ? */ "0,8 2,10 4,10 6,8 6,6 3,4 3,3;3,0 3,1",
    /* ' */ "3,10 3,8",
    /* * */ "3,2 3,8;0,3 6,7;0,7 6,3",
    /* / */ "0,0 6,10",
};
static const char glyph_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.,:!?'*/";

void gfx_clear(gfx_band_t *b, uint16_t colour)
{
    uint16_t *p = b->px;
    for (int i = 0, n = GFX_W * b->h; i < n; i++) p[i] = colour;
}

void gfx_pixel(gfx_band_t *b, int x, int y, uint16_t colour)
{
    int ly = y - b->y0;
    if (x < 0 || x >= GFX_W || ly < 0 || ly >= b->h) return;
    b->px[ly * GFX_W + x] = colour;
}

void gfx_fill_rect(gfx_band_t *b, int x, int y, int w, int h, uint16_t colour)
{
    int x0 = x < 0 ? 0 : x, x1 = x + w > GFX_W ? GFX_W : x + w;
    int y0 = y - b->y0, y1 = y0 + h;
    if (y0 < 0) y0 = 0;
    if (y1 > b->h) y1 = b->h;
    for (int yy = y0; yy < y1; yy++) {
        uint16_t *row = b->px + yy * GFX_W;
        for (int xx = x0; xx < x1; xx++) row[xx] = colour;
    }
}

void gfx_line(gfx_band_t *b, int x0, int y0, int x1, int y1, uint16_t colour)
{
    int dx = x1 - x0, dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    int err = dx - dy;
    for (;;) {
        gfx_pixel(b, x0, y0, colour);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

static const char *glyph_for(char c)
{
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (!c) return NULL;
    const char *p = strchr(glyph_chars, c);
    return p ? glyphs[p - glyph_chars] : NULL;
}

/* Read "x,y" starting at *s; advance *s past it. Returns 0 at end of stroke. */
static int next_point(const char **s, int *x, int *y)
{
    const char *p = *s;
    while (*p == ' ') p++;
    if (!*p || *p == ';') { *s = p; return 0; }

    int sign = 1, v = 0;
    if (*p == '-') { sign = -1; p++; }
    while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
    *x = sign * v;
    if (*p == ',') p++;
    sign = 1; v = 0;
    if (*p == '-') { sign = -1; p++; }
    while (*p >= '0' && *p <= '9') v = v * 10 + (*p++ - '0');
    *y = sign * v;
    *s = p;
    return 1;
}

int gfx_text_width(const char *s, int scale)
{
    int n = 0;
    for (const char *p = s; *p; p++) n++;
    return n > 0 ? (n * GFX_ADVANCE - (GFX_ADVANCE - 6)) * scale : 0;
}

void gfx_text(gfx_band_t *b, int x, int y, const char *s, int scale, uint16_t colour)
{
    if (scale < 1) scale = 1;
    /* Skip the whole string if no part of its cell reaches this band. */
    int top = y, bot = y + GFX_GLYPH_H * scale;
    if (bot < b->y0 || top >= b->y0 + b->h) return;

    for (const char *c = s; *c; c++, x += GFX_ADVANCE * scale) {
        const char *g = glyph_for(*c);
        if (!g) continue;
        const char *p = g;
        while (*p) {
            int px, py, first = 1, lx = 0, ly = 0;
            while (next_point(&p, &px, &py)) {
                /* glyph space has y up; screen has y down */
                int sx = x + px * scale;
                int sy = y + (GFX_GLYPH_H - py) * scale;
                if (!first) gfx_line(b, lx, ly, sx, sy, colour);
                lx = sx; ly = sy; first = 0;
            }
            if (*p == ';') p++;
        }
    }
}

void gfx_text_center(gfx_band_t *b, int cx, int y, const char *s, int scale, uint16_t colour)
{
    gfx_text(b, cx - gfx_text_width(s, scale) / 2, y, s, scale, colour);
}
