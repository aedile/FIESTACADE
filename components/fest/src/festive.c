/*
 * festive.c - papel picado, confetti and a dancing stage, all drawn procedurally.
 *
 * Ported from NESTOR (the Fiesta Entertainment System medal) and re-fitted from
 * that project's 256x240 landscape frame to this one's 240x280 portrait. The
 * sprites are character tables rather than artwork, so nothing here ships a
 * bitmap or borrows one.
 */
#include "fest.h"
#include <stdbool.h>
#include <stddef.h>

#define L  0
#define R  FB_W
#define CX (FB_W / 2)

const uint8_t fiesta_colours[6] = {
    CUBE(5,1,3), CUBE(0,5,4), CUBE(5,5,0), CUBE(1,5,1), CUBE(5,3,0), CUBE(3,0,4)
};

void fest_papel_picado(int frame)
{
    static const int8_t sway[8] = { 0, 1, 2, 1, 0, -1, -2, -1 };
    for (int x = L; x < R; x++) {
        int d = x - CX;
        fest_px(x, 6 + (d * d) / 1400, UI_GREY);            /* the string sags in the middle */
    }
    for (int i = 0; i < 8; i++) {
        int x = L + 3 + i * 29 + sway[((frame >> 3) + i) & 7];
        int d = x + 9 - CX, y = 7 + (d * d) / 1400;
        uint8_t c = fiesta_colours[i % 6];
        fest_fill(x, y, 18, 16, c);
        for (int k = 0; k < 4; k++) fest_fill(x + 8, y + 3 + k * 3, 2, 1, UI_BLACK);   /* punched pattern */
        fest_fill(x + 5, y + 6, 8, 1, UI_BLACK);
        fest_fill(x + 5, y + 10, 8, 1, UI_BLACK);
        fest_fill(x, y + 13, 18, 3, c);
        for (int k = 0; k < 18; k += 3) fest_px(x + k, y + 16, c);                     /* scalloped edge */
    }
}

void fest_confetti(int frame)
{
    static const int8_t sway[16] = { 0, 1, 1, 2, 2, 2, 1, 1, 0, -1, -1, -2, -2, -2, -1, -1 };
    for (int i = 0; i < 56; i++) {
        uint32_t h = (uint32_t)(i + 1) * 2654435761u;
        int speed = 1 + (h & 1), period = FB_H + 40;
        int y = (int)((((h >> 8) % period) + frame * speed / 2) % period) - 20;
        int x = L + (int)((h >> 16) % (R - L)) + sway[((frame >> 2) + i) & 15];
        uint8_t c = fiesta_colours[i % 6];
        fest_px(x, y, c); fest_px(x + 1, y, c);
        if (((frame >> 3) + i) & 1) { fest_px(x, y + 1, c); fest_px(x + 1, y + 1, c); }
    }
}

/* 20x24 folklorico dancer (skirt swept to one side, mirrored for the other) and a
 * 16x24 mariachi whose strumming arm shifts. Letters index the colour key below. */
static const char *const dancer[24] = {
    "....rryrr...........",
    "...rhhhhhr..........",
    "...hhhhhhh..........",
    "....kkkkk...........",
    "....kkkkk...........",
    ".....kkk............",
    "...wwwwwww..........",
    "..kwwwwwwwk.........",
    ".kk.wwwww.kk........",
    "k...wwwww...k.......",
    "....ppppp...........",
    "...ppppppp..........",
    "..pppppppppp........",
    ".pppppppppppppp.....",
    "pppppppppppppppppp..",
    "ppgppppgppppgppppgp.",
    "pppppppppppppppppppp",
    "ppppppppppppppppppp.",
    ".pppppppppppppppp...",
    "..pppppppppppp......",
    "....k...k...........",
    "....k...k...........",
    "...dd...dd..........",
    "....................",
};
static const char *const mariachi[24] = {
    "......ssss......",
    ".....ssssss.....",
    "..ssssssssssss..",
    ".syyyyyyyyyyyys.",
    "..ssssssssssss..",
    ".....kkkkkk.....",
    ".....kkkkkk.....",
    ".....kkhhkk.....",
    "......kkkk......",
    "....ddwwwwdd....",
    "...dddwddwddd...",
    "..kddddddddddk..",
    "..k.ddddddddd.k.",
    "....dddttttd....",
    "...ttttttttttt..",
    "...tttttdttttt..",
    "....ttttttttt...",
    ".....ddddddd....",
    ".....ddd.ddd....",
    ".....ddd.ddd....",
    ".....ddd.ddd....",
    ".....ddd.ddd....",
    "....dddd.dddd...",
    "................",
};

static uint8_t key(char ch, uint8_t skirt)
{
    switch (ch) {
    case 'k': return CUBE(5,4,2);   /* skin */
    case 'h': return CUBE(1,0,0);   /* hair, moustache */
    case 'r': return CUBE(5,0,0);   /* flowers */
    case 'y': return CUBE(5,5,0);   /* sombrero braid */
    case 'w': return UI_WHITE;
    case 'p': return skirt;
    case 'g': return CUBE(1,5,1);
    case 'd': return CUBE(1,1,3);   /* charro suit, shoes: navy */
    case 's': return CUBE(4,3,1);   /* sombrero */
    case 't': return CUBE(5,4,1);   /* guitar */
    }
    return 0;
}

#define SCALE 2
static void sprite(const char *const *rows, int w, int x0, int y0, bool mirror,
                   int shift_from, int shift_by, uint8_t skirt)
{
    for (int y = 0; y < 24; y++) {
        int dx = (shift_from && y >= shift_from && y < shift_from + 4) ? shift_by : 0;
        for (int x = 0; x < w; x++) {
            char ch = rows[y][x];
            if (ch == '.') continue;
            int sx = x0 + ((mirror ? w - 1 - x : x) + dx) * SCALE, sy = y0 + y * SCALE;
            fest_fill(sx, sy, SCALE, SCALE, key(ch, skirt));
        }
    }
}

void fest_dancers(int frame, int floor_y)
{
    int beat = (frame >> 3) & 3;                    /* ~2 Hz step */
    int bounce = (beat == 1 || beat == 3) ? -3 : 0;
    bool swing = (frame >> 4) & 1;                  /* skirt / strum alternates ~2x a second */
    int top = floor_y - 24 * SCALE;
    sprite(dancer,   20,   4, top + bounce,  swing, 0, 0, CUBE(5,1,3));
    sprite(mariachi, 16,  62, top - bounce,  false, 13, swing ? 1 : 0, 0);
    sprite(dancer,   20, 126, top + bounce, !swing, 0, 0, CUBE(0,5,4));
    sprite(mariachi, 16, 190, top - bounce,   true, 13, swing ? 0 : 1, 0);
    fest_fill(L, floor_y, R - L, 1, CUBE(4,3,1));   /* the floor */
}

/* ---------------------------------------------------------------------------
 * Splash set pieces. All original and drawn from primitives: no photographs,
 * no traced logos, and nothing borrowed from a game.
 * ------------------------------------------------------------------------- */

/* ---------------------------------------------------------------------------
 * Night scene: stars, a skyline with the Tower of the Americas, and fireworks.
 * Ported from NESTOR's splash. Geometry re-fitted from that project's 256-wide
 * frame (L=8, CX=128) to this one's 240 (L=0, CX=120), so every absolute x
 * moves left by eight and everything measured from centre carries over.
 * ------------------------------------------------------------------------- */

#define GROUND 204

static uint32_t rnd_state = 12345;
static uint32_t rnd(void) { rnd_state = rnd_state * 1664525u + 1013904223u; return rnd_state >> 8; }
static int rndn(int n) { return (int)(rnd() % (uint32_t)n); }

/* ~100 stars in three brightness tiers, each twinkling on its own phase */
void fest_stars(int frame)
{
    static const uint8_t tier[3] = { CUBE(1,1,2), CUBE(3,3,4), UI_WHITE };
    for (int i = 0; i < 100; i++) {
        uint32_t h = (uint32_t)i * 2654435761u;
        int x = L + (int)((h >> 8) % (uint32_t)(R - L)), y = (int)((h >> 20) % 150);
        int t = (int)((h >> 4) % 10);
        int phase = (frame + (int)(h & 63)) >> 3;
        int tw = (phase * 5 + i) % 7;
        if (t < 6) { if (tw > 1) fest_px(x, y, tier[0]); }
        else if (t < 9) { fest_px(x, y, tw > 3 ? tier[1] : tier[0]); }
        else {
            fest_px(x, y, tier[2]);
            if (tw > 4) {                      /* the brightest flare into a sparkle */
                fest_px(x - 1, y, tier[1]); fest_px(x + 1, y, tier[1]);
                fest_px(x, y - 1, tier[1]); fest_px(x, y + 1, tier[1]);
            }
        }
    }
}

void fest_skyline(void)
{
    /* x, width, height - absolute, already shifted left by eight from NESTOR */
    static const uint8_t bld[][3] = {
        {0,30,26},{30,18,40},{48,24,34},{70,14,48},{162,20,44},{182,26,30},{208,16,38},{224,16,24}
    };
    uint8_t dark = CUBE(0,0,1), win = CUBE(5,5,2);
    for (size_t i = 0; i < sizeof bld / sizeof *bld; i++) {
        fest_fill(bld[i][0], GROUND - bld[i][2], bld[i][1], bld[i][2], dark);
        for (int y = GROUND - bld[i][2] + 3; y < GROUND - 2; y += 5)
            for (int x = bld[i][0] + 2; x < bld[i][0] + bld[i][1] - 2; x += 5)
                if (((x * 7 + y * 13) / 5) % 3) fest_px(x, y, win);
    }

    /* Tower of the Americas: tan concrete shaft flared at the base, a tophouse of
     * restaurant and observation levels with lit windows under a wide crown, spire on top */
    uint8_t tan = CUBE(4,3,2), tan_dk = CUBE(3,2,1), tan_lt = CUBE(5,4,3), glass = CUBE(0,0,2);
    fest_fill(CX - 7, 92, 14, GROUND - 92, tan);
    fest_fill(CX - 7, 92, 3, GROUND - 92, tan_lt);            /* lit side */
    fest_fill(CX + 4, 92, 3, GROUND - 92, tan_dk);            /* shaded side */
    for (int y = GROUND - 24; y < GROUND; y++) {              /* base flare */
        int w = (y - (GROUND - 24)) / 3;
        fest_fill(CX - 7 - w, y, 14 + 2 * w, 1, tan);
        fest_px(CX - 7 - w, y, tan_lt); fest_px(CX + 6 + w, y, tan_dk);
    }
    fest_fill(CX - 26, 56, 52, 5, tan_lt);                    /* crown */
    fest_fill(CX - 24, 61, 48, 3, tan);
    fest_fill(CX - 22, 64, 44, 8, glass);                     /* restaurant level */
    for (int x = CX - 20; x < CX + 20; x += 4) fest_fill(x, 66, 2, 4, win);
    fest_fill(CX - 22, 72, 44, 3, tan);
    fest_fill(CX - 20, 75, 40, 7, glass);                     /* observation level */
    for (int x = CX - 18; x < CX + 18; x += 4) fest_fill(x, 77, 2, 3, win);
    fest_fill(CX - 20, 82, 40, 3, tan);
    for (int y = 85; y < 92; y++)                             /* underside taper */
        fest_fill(CX - 17 + (y - 85) * 3 / 2, y, 34 - (y - 85) * 3, 1, tan_dk);
    fest_fill(CX - 1, 30, 2, 26, tan_lt);                     /* spire */
    fest_fill(CX - 2, 44, 4, 2, tan);
    fest_fill(L, GROUND, R - L, FB_H - GROUND, CUBE(0,0,1));  /* ground */
}

/* aircraft beacon on the spire tip; draw last so nothing covers it */
void fest_beacon(int frame)
{
    if ((frame / 30) & 1) { fest_px(CX - 1, 29, CUBE(5,0,0)); fest_px(CX, 29, CUBE(5,0,0)); }
}

/* ---- fireworks: rockets that arc up and burst into drifting sparks ---- */
typedef struct { int16_t x, y, vx, vy; uint8_t life, colour; } spark_t;   /* 1/16 px */
#define SPARKS 160
static spark_t sparks[SPARKS];
typedef struct { int16_t x, y, vy; uint8_t colour; bool live; } rocket_t;
static rocket_t rockets[3];

static void burst(int x, int y, uint8_t colour)
{
    int n = 0;
    for (int i = 0; i < SPARKS && n < 64; i++) {
        if (sparks[i].life) continue;
        int a = rndn(64), sp = 12 + rndn(30);
        static const int8_t tbl[16] = { 0,12,23,30,32,30,23,12,0,-12,-23,-30,-32,-30,-23,-12 };
        sparks[i] = (spark_t){ (int16_t)(x * 16), (int16_t)(y * 16),
                               (int16_t)(tbl[(a + 4) & 15] * sp / 32),
                               (int16_t)(tbl[a & 15] * sp / 32),
                               (uint8_t)(55 + rndn(35)), colour };
        n++;
    }
}

void fest_fireworks(int frame)
{
    if (frame > 20 && rndn(30) == 0)
        for (int i = 0; i < 3; i++) if (!rockets[i].live) {
            rockets[i] = (rocket_t){ (int16_t)((L + 30 + rndn(R - L - 60)) * 16),
                                     (int16_t)(GROUND * 16),
                                     (int16_t)(-(58 + rndn(16))),
                                     fiesta_colours[rndn(6)], true };
            break;
        }
    for (int i = 0; i < 3; i++) {
        rocket_t *r = &rockets[i];
        if (!r->live) continue;
        r->y = (int16_t)(r->y + r->vy); r->vy++;
        fest_px(r->x / 16, r->y / 16, UI_WHITE);
        fest_px(r->x / 16, r->y / 16 + 1, UI_GREY);
        if (r->vy >= -4) { r->live = false; burst(r->x / 16, r->y / 16, r->colour); }
    }
    for (int i = 0; i < SPARKS; i++) {
        spark_t *s = &sparks[i];
        if (!s->life) continue;
        s->x = (int16_t)(s->x + s->vx); s->y = (int16_t)(s->y + s->vy); s->vy++;
        s->life--;
        s->vx = (int16_t)(s->vx - s->vx / 24);      /* air drag keeps bursts round */
        s->vy = (int16_t)(s->vy - s->vy / 24);
        uint8_t c = s->colour;
        if (s->life < 10) c = UI_GREY;
        else if (s->life < 22) c = CUBE((s->colour / 30) / 2, ((s->colour / 5) % 6) / 2, (s->colour % 5) / 2);
        int sx = s->x / 16, sy = s->y / 16;
        fest_px(sx, sy, c);
        if (s->life > 22) { fest_px(sx + 1, sy, c); fest_px(sx, sy + 1, c); fest_px(sx + 1, sy + 1, c); }
        if (sy >= GROUND) s->life = 0;
    }
}

static const char *const pinata[16] = {
    ".......yy.......",
    "......yggy......",
    "...r..gggg..r...",
    "...rr.gggg.rr...",
    "....rrggggrr....",
    "..bbrrrggrrrbb..",
    ".bbbbrrrrrrbbbb.",
    "bbbbppppppppbbbb",
    ".bbbppppppppbbb.",
    "...ppppppppppp..",
    "...yyppppppyy...",
    "....yyyppyyy....",
    ".....yy..yy.....",
    "....gg....gg....",
    "...gg......gg...",
    "..gg........gg..",
};

void fest_pinata(int frame, int cx, int top_y)
{
    static const int8_t swing[8] = { 0, 2, 3, 2, 0, -2, -3, -2 };
    int s = swing[(frame >> 2) & 7];
    int x0 = cx - 16 + s * 2, y0 = top_y + 16;
    for (int y = 0; y < 12; y++) fest_px(cx + s * y / 12, top_y + y, UI_GREY);   /* the cord */
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            char ch = pinata[y][x];
            if (ch == '.') continue;
            uint8_t c = ch == 'r' ? CUBE(5,0,1) : ch == 'b' ? CUBE(0,3,4)
                      : ch == 'g' ? CUBE(1,5,1) : ch == 'y' ? CUBE(5,5,0) : CUBE(5,2,0);
            fest_fill(x0 + x * 2, y0 + y * 2, 2, 2, c);
        }
}

void fest_cabinet(int frame, int cx, int base_y)
{
    uint8_t shell = CUBE(1,1,2), trim = CUBE(4,0,1);
    int w = 74, h = 116, x = cx - w / 2, y = base_y - h;
    fest_fill(x, y, w, h, shell);                       /* body */
    fest_fill(x + 3, y + 4, w - 6, 16, trim);           /* header, lit */
    for (int i = 0; i < 5; i++)
        fest_fill(x + 8 + i * 12, y + 8, 8, 8, ((frame >> 2) + i) & 1 ? CUBE(5,5,2) : CUBE(5,3,0));
    fest_fill(x + 6, y + 26, w - 12, 44, UI_BLACK);     /* screen bezel */
    /* something playing on it: a drifting starfield with a blob dodging about */
    for (int i = 0; i < 18; i++) {
        uint32_t hh = (uint32_t)(i + 3) * 2246822519u;
        int sx = x + 8 + (int)(((hh >> 7) % (uint32_t)(w - 16) + frame) % (uint32_t)(w - 16));
        int sy = y + 28 + (int)((hh >> 15) % 40);
        fest_px(sx, sy, CUBE(2,2,4));
    }
    int bx = x + w / 2 + ((frame >> 1) % 24) - 12;
    fest_fill(bx, y + 58, 6, 5, CUBE(1,5,1));
    fest_fill(x + 6, y + 74, w - 12, 12, CUBE(2,2,3));  /* control panel */
    fest_fill(x + 16, y + 77, 5, 5, CUBE(5,0,0));       /* stick */
    fest_fill(x + 34, y + 78, 4, 4, CUBE(5,5,0));
    fest_fill(x + 44, y + 78, 4, 4, CUBE(0,3,5));
    fest_fill(x, base_y - 26, w, 26, CUBE(1,1,1));      /* the plinth */
}
