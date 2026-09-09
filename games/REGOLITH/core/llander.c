/*
 * llander.c - Atari Lunar Lander board: memory map, Digital Vector Generator, sound.
 * The 6502 is the instruction-stepped core in m6502fast.h. The DVG is Asteroids' (AEROLITE).
 */
#include "llander.h"
#include <string.h>
#ifdef LL_DEBUG
#include <stdio.h>
#endif

static ll_roms_t roms;
static uint8_t ram[0x400];               /* 0x0000-0x03FF work RAM, mirrored through 0x1FFF */
static uint8_t vecram[0x800];            /* 0x4000-0x47FF display list */
static uint8_t dsw1 = 0xa0;              /* English, 1 coin 1 play, 900 fuel units */
static ll_input_t input;
static uint32_t frame_count;
static uint32_t total_cycles;
static int32_t cycle_debt;
uint32_t ll_dbg_snd_writes[256];
uint32_t ll_dbg_vggo;
#ifdef LL_DEBUG
uint32_t ll_dbg_pc_hist[0x10000];
#endif

/* ---- vector generator ---- */
static ll_line_t lines_a[LL_MAX_LINES], lines_b[LL_MAX_LINES];
static ll_line_t *lines_draw = lines_a, *lines_done = lines_b;
static int nlines_draw, nlines_done;
static uint32_t dvg_go_cycle, dvg_busy_cycles;   /* the DVG holds VG_HALT low while it runs */

/* ---- sound latch ---- */
static uint8_t snd_latch;                /* bits 0-2 thrust level, 3 explosion, 4 tone 3 kHz, 5 tone 6 kHz */

/* one word of the DVG's address space: vector RAM at 0x4000, vector ROM at 0x4800 */
static inline uint16_t vec_word(uint16_t pc)
{
    uint16_t a = (uint16_t)(pc << 1);                  /* byte offset from 0x4000 */
    if (a < 0x800) return (uint16_t)(vecram[a] | (vecram[a + 1] << 8));
    if (a < 0x2000) return (uint16_t)(roms.vecrom[a - 0x800] | (roms.vecrom[a - 0x800 + 1] << 8));
    return 0;                                          /* unmapped: reads as zero */
}

static inline void emit(int x0, int y0, int x1, int y1, int bright)
{
    if (nlines_draw >= LL_MAX_LINES) return;
    ll_line_t *l = &lines_draw[nlines_draw++];
    l->x0 = (int16_t)x0; l->y0 = (int16_t)y0;
    l->x1 = (int16_t)x1; l->y1 = (int16_t)y1;
    l->bright = (uint8_t)bright;
}

/*
 * One VCTR or SVEC. The hardware walks the vector with a pair of cascaded 7497 rate
 * multipliers: over `fin` clocks the x counter steps `fin * mx / 4096` times, and likewise
 * for y, which is what gives the line its slope. We can compute the endpoint directly.
 */
static void dvg_vector(int scale, int dvx, int dvy, int z, int *xpos, int *ypos)
{
    int fin = (2 << (scale & 0x0f)) & 0x7ff;
    int mx = (dvx << 2) & 0xfff;
    int my = (dvy << 2) & 0xfff;
    int sx = (fin * mx) >> 12;
    int sy = (fin * my) >> 12;
    int nx = *xpos + ((dvx & 0x400) ? -sx : sx);
    int ny = *ypos + ((dvy & 0x400) ? -sy : sy);
    if (z) emit(*xpos, *ypos, nx, ny, z);
    *xpos = nx; *ypos = ny;
    dvg_busy_cycles += (uint32_t)fin;      /* the DVG spends `fin` CPU cycles on this vector */
}

static void dvg_run(void)
{
    uint16_t pc = 0, stack[4];
    int sp = 0, scale = 0, xpos = 0, ypos = 0;
    nlines_draw = 0;
    dvg_busy_cycles = 0;
    /* the display list is the CPU's own data: a corrupt one must not hang us */
    for (int guard = 0; guard < 8192; guard++) {
        uint16_t w0 = vec_word(pc++);
        int op = w0 >> 12;
        if (op <= 0x0a) {
            uint16_t w1 = vec_word(pc++);
            int dvy = w0 & 0xfff, dvx = w1 & 0xfff, z = w1 >> 12;
            if (op == 0x0a) { xpos = dvx; ypos = dvy; scale = z; }   /* LABS */
            else dvg_vector(scale + op, dvx, dvy, z, &xpos, &ypos);  /* VCTR */
        } else if (op == 0x0b) {                                     /* HALT */
            break;
        } else if (op == 0x0c) {                                     /* JSRL */
            stack[sp++ & 3] = pc; pc = w0 & 0xfff;
        } else if (op == 0x0d) {                                     /* RTSL */
            pc = stack[--sp & 3];
        } else if (op == 0x0e) {                                     /* JMPL */
            pc = w0 & 0xfff;
        } else {                                                     /* SVEC, one word */
            int dvy = w0 & 0xf00;
            int dvx = (w0 & 0x00f) << 8;
            int z = (w0 >> 4) & 0x0f;
            /* the short vector borrows three scale bits from the top bits of dx and dy */
            int adj = ((dvy & 0x800) >> 11) | (((dvx & 0x800) ^ 0x800) >> 10) | ((dvx & 0x800) >> 9);
            dvg_vector(scale + adj, dvx, dvy, z, &xpos, &ypos);
        }
    }
    /* swap: the list the CPU just built becomes the one the display shows */
    ll_line_t *t = lines_done; lines_done = lines_draw; lines_draw = t;
    nlines_done = nlines_draw;
}

/* ---- bus ---- */
static uint8_t bus_read(uint16_t addr)
{
    uint16_t a = addr & 0x7fff;                /* A15 is not decoded */
    if (a < 0x2000) return ram[a & 0x3ff];
    if (a >= 0x6000) return roms.rom[a - 0x6000];
    if (a >= 0x4800 && a < 0x6000) return roms.vecrom[a - 0x4800];
    if (a >= 0x4000 && a < 0x4800) return vecram[a - 0x4000];
    switch (a & 0xff00) {
        case 0x2000: {                         /* IN0, the whole port at once */
            /*
             * Read from the program itself: bit 0 is VG_HALT, which the display routine spins
             * on until the DVG has finished the last list; bit 1 is the self-test switch,
             * which the reset code tests and the display routine hangs on if it is off (low);
             * bit 2 low starts a lockout timer, so it is the slam switch; bit 6 is the 3 kHz
             * clock the program waits a full edge of.
             */
            uint8_t v = 0x06;
            if (total_cycles - dvg_go_cycle >= dvg_busy_cycles) v |= 0x01;
            if (total_cycles & 0x100) v |= 0x40;
            return v;
        }
        case 0x2400: {                         /* IN1, one bit per address in bit 7 */
            int bit = a & 7, v;
            switch (bit) {
                case 0: v = input.start1; break;   /* the program sets a flag off this one */
                case 1: v = input.coin1; break;    /* the three coin switches are bits 1-3 */
                case 4: v = input.abort_; break;
                case 5: v = input.select; break;
                case 6: v = input.right; break;
                case 7: v = input.left; break;
                default: v = 0; break;
            }
            return v ? 0x80 : 0x7f;
        }
        case 0x2800: {                         /* DSW1 through a 74LS253: two bits per address */
            int pair = 3 - (a & 3);
            return (uint8_t)(0xfc | ((dsw1 >> (pair * 2)) & 3));
        }
        case 0x2c00:                           /* the throttle, through an ADC */
            return input.thrust;
        default: return 0xff;
    }
}

static void bus_write(uint16_t addr, uint8_t data)
{
    uint16_t a = addr & 0x7fff;
    if (a < 0x2000) { ram[a & 0x3ff] = data; return; }
    if (a >= 0x4000 && a < 0x4800) { vecram[a - 0x4000] = data; return; }
    switch (a & 0xff00) {
        case 0x3000:                           /* VGGO: draw the list the CPU just built */
            dvg_go_cycle = total_cycles;
            ll_dbg_vggo++;
            dvg_run();
            return;
        case 0x3200: return;                   /* lamps and the coin counter */
        case 0x3400: return;                   /* watchdog */
        case 0x3c00:                           /* the sound latch, as one byte */
            snd_latch = data;
            ll_dbg_snd_writes[data]++;
            return;
        case 0x3e00: return;                   /* noise reset */
        default: return;
    }
}

#define M6502F_READ(a)     bus_read(a)
#define M6502F_WRITE(a, v) bus_write(a, v)
#include "m6502fast.h"

static m6502f_t cpu;

/* ---- public ---- */
void ll_reset(void)
{
    memset(ram, 0, sizeof(ram));
    memset(vecram, 0, sizeof(vecram));
    memset(&input, 0, sizeof(input));
    nlines_draw = nlines_done = 0;
    total_cycles = 0; cycle_debt = 0;
    dvg_go_cycle = 0; dvg_busy_cycles = 0;
    snd_latch = 0;
    m6502f_reset(&cpu);
}

void ll_init(const ll_roms_t *r) { roms = *r; ll_reset(); }
void ll_set_dips(uint8_t d) { dsw1 = d; }
ll_input_t *ll_input(void) { return &input; }

static void run_cycles(int32_t n)
{
    int32_t budget = n - cycle_debt;
    while (budget > 0) {
#ifdef LL_DEBUG
        ll_dbg_pc_hist[cpu.pc]++;
        { static uint16_t ring[96]; static int ri, trapped;
          ring[ri++ % 96] = cpu.pc;
          if (cpu.pc == 0x7AEB && !trapped) { trapped = 1;
              printf("HALT at frame %u, cycles %u; last PCs:", frame_count, total_cycles);
              for (int k = 0; k < 96; k++) printf(" %04X", ring[(ri + k) % 96]);
              printf("\nzp 73=%02X 74=%02X 8A=%02X 00=%02X C1=%02X C2=%02X\n", ram[0x73], ram[0x74], ram[0x8a], ram[0], ram[0xc1], ram[0xc2]); } }
#endif
        int cy = m6502f_step(&cpu);
        total_cycles += (uint32_t)cy;
        budget -= cy;
    }
    cycle_debt = -budget;
}

void ll_run_frame(void)
{
    for (int i = 0; i < LL_NMIS_PER_FRAME; i++) {
        run_cycles(LL_NMI_CYCLES);
        /* the periodic interrupt is an NMI, and is inhibited only by the self-test switch */
        total_cycles += (uint32_t)m6502f_nmi(&cpu);
    }
    frame_count++;
}

const ll_line_t *ll_lines(int *count) { *count = nlines_done; return lines_done; }

uint16_t ll_pc(void) { return cpu.pc; }
uint32_t ll_frame_count(void) { return frame_count; }
const uint8_t *ll_ram(void) { return ram; }

/* ---- sound ----
 * Discrete on the board: thrust is the noise source through a three-bit level, the explosion
 * is the same noise through a low-pass with a long tail, and the two beeps are square waves.
 * Fixed-point, because the ESP32-C6 has no FPU.
 */
static uint32_t sr_noise = 0x1u;
static int32_t lp_thrust, lp_explode, thrust_lvl, explode_amp;
static uint32_t ph_3k, ph_6k;
static uint8_t prev_explode;

static inline int32_t noise_step(void)
{                                        /* 17-bit maximal LFSR, as on the board */
    uint32_t bit = ((sr_noise >> 0) ^ (sr_noise >> 3)) & 1;
    sr_noise = (sr_noise >> 1) | (bit << 16);
    return (sr_noise & 1) ? 32767 : -32767;
}

void ll_render_audio(int16_t *buf, int samples, int rate)
{
    const uint32_t inc_3k = (uint32_t)(((uint64_t)3000 << 32) / (uint32_t)rate);
    const uint32_t inc_6k = (uint32_t)(((uint64_t)6000 << 32) / (uint32_t)rate);
    const int32_t target = (snd_latch & 7) * (32767 / 7);
    const uint8_t explode = (snd_latch >> 3) & 1;
    if (explode && !prev_explode) explode_amp = 32767;
    prev_explode = explode;

    for (int i = 0; i < samples; i++) {
        int32_t out = 0;
        int32_t n = noise_step();

        /* thrust: the level moves toward the DAC's value, so the throttle sounds like one */
        thrust_lvl += (target - thrust_lvl) >> 8;
        if (thrust_lvl > 64) {
            lp_thrust += (n - lp_thrust) >> 5;
            out += (lp_thrust * thrust_lvl) >> 16;
        }
        /* the crash: a burst of low noise dying away */
        if (explode_amp > 8) {
            lp_explode += (n - lp_explode) >> 3;
            out += (lp_explode * explode_amp) >> 15;
            explode_amp -= (explode_amp >> 13) + 1;
        }
        if (snd_latch & 0x10) { ph_3k += inc_3k; out += (ph_3k & 0x80000000u) ? 5000 : -5000; }
        if (snd_latch & 0x20) { ph_6k += inc_6k; out += (ph_6k & 0x80000000u) ? 5000 : -5000; }

        if (out > 32767) out = 32767; else if (out < -32768) out = -32768;
        buf[i] = (int16_t)out;
    }
}
