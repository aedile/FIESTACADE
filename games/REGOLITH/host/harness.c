/*
 * harness.c - run Lunar Lander on the host; frames to PPM, audio to WAV.
 * usage: harness <outdir> [seconds] [--every S] [--wav f] [--script "T:key=val,..."] [--dsw X]
 * script keys: coin start select abort thrust(0-255) left right
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "llander.h"
#include "llander_roms.h"

#define W 640
#define H 400

typedef struct { double t; char key[8]; int val; } event_t;
static uint8_t fb[W * H];

static void plot(int x, int y, int b)
{
    if (x < 0 || y < 0 || x >= W || y >= H) return;
    int v = fb[y * W + x] + b;
    fb[y * W + x] = (uint8_t)(v > 255 ? 255 : v);
}

static void draw_line(int x0, int y0, int x1, int y1, int b)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (int guard = 0; guard < 4000; guard++) {
        plot(x0, y0, b);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s outdir [seconds] [--every S] [--wav f] [--script s] [--dsw X]\n", argv[0]); return 1; }
    const char *outdir = argv[1];
    double seconds = argc > 2 && argv[2][0] != '-' ? atof(argv[2]) : 20;
    double every = 1.0; const char *wav_path = NULL; int dsw = 0x00;
    event_t evs[64]; int nev = 0;
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--every") && i + 1 < argc) every = atof(argv[++i]);
        else if (!strcmp(argv[i], "--wav") && i + 1 < argc) wav_path = argv[++i];
        else if (!strcmp(argv[i], "--dsw") && i + 1 < argc) dsw = (int)strtol(argv[++i], NULL, 0);
        else if (!strcmp(argv[i], "--script") && i + 1 < argc) {
            char *sc = strdup(argv[++i]);
            for (char *tok = strtok(sc, ","); tok && nev < 64; tok = strtok(NULL, ",")) {
                double t; char key[8]; int val;
                if (sscanf(tok, "%lf:%7[a-z0-9]=%i", &t, key, &val) == 3) { evs[nev].t = t; strcpy(evs[nev].key, key); evs[nev].val = val; nev++; }
            }
        }
    }
    ll_roms_t roms = { ll_rom, ll_vecrom };
    ll_init(&roms);
    ll_set_dips((uint8_t)dsw);
    ll_input_t *in = ll_input();

    FILE *wav = NULL; const int rate = 20050; uint32_t wav_samples = 0;
    if (wav_path) { wav = fopen(wav_path, "wb"); uint8_t hdr[44] = {0}; fwrite(hdr, 1, 44, wav); }
    static int16_t abuf[4096];
    const double fps = (double)LL_CPU_CLOCK / LL_CYCLES_PER_FRAME;
    int frames = (int)(seconds * fps), saved = 0;
    double next_save = 0, audio_acc = 0;
    int gx0 = 99999, gy0 = 99999, gx1 = -99999, gy1 = -99999;

    for (int f = 0; f < frames; f++) {
        double now = f / fps;
        for (int e = 0; e < nev; e++) {
            if (evs[e].t <= now && evs[e].t > now - 1.0 / fps) {
                const char *k = evs[e].key; int v = evs[e].val;
                if (!strcmp(k, "coin")) in->coin1 = v; else if (!strcmp(k, "start")) in->start1 = v;
                else if (!strcmp(k, "select")) in->select = v; else if (!strcmp(k, "thrust")) in->thrust = (uint8_t)v;
                else if (!strcmp(k, "left")) in->left = v; else if (!strcmp(k, "right")) in->right = v;
                else if (!strcmp(k, "abort")) in->abort_ = v;
            }
        }
        ll_run_frame();
        if (wav) {
            audio_acc += rate / fps; int n = (int)audio_acc; audio_acc -= n;
            ll_render_audio(abuf, n, rate); fwrite(abuf, 2, n, wav); wav_samples += n;
        }
        int n; const ll_line_t *ls = ll_lines(&n);
        for (int i = 0; i < n; i++) {
            if (ls[i].x0 < gx0) gx0 = ls[i].x0; if (ls[i].x0 > gx1) gx1 = ls[i].x0;
            if (ls[i].x1 < gx0) gx0 = ls[i].x1; if (ls[i].x1 > gx1) gx1 = ls[i].x1;
            if (ls[i].y0 < gy0) gy0 = ls[i].y0; if (ls[i].y0 > gy1) gy1 = ls[i].y0;
            if (ls[i].y1 < gy0) gy0 = ls[i].y1; if (ls[i].y1 > gy1) gy1 = ls[i].y1;
        }
        if (now >= next_save) {
            memset(fb, 0, sizeof(fb));
            for (int i = 0; i < n; i++) {          /* the beam window -> W x H, y flipped */
                #define CX(v) (((v) - LL_X_MIN) * W / (LL_X_MAX - LL_X_MIN))
                #define CY(v) (H - 1 - ((v) - LL_Y_MIN) * H / (LL_Y_MAX - LL_Y_MIN))
                int x0 = CX(ls[i].x0), x1 = CX(ls[i].x1);
                int y0 = CY(ls[i].y0), y1 = CY(ls[i].y1);
                draw_line(x0, y0, x1, y1, 60 + ls[i].bright * 13);
            }
            char path[512]; snprintf(path, sizeof(path), "%s/frame_%03d.ppm", outdir, saved);
            FILE *fp = fopen(path, "wb");
            if (fp) { fprintf(fp, "P6\n%d %d\n255\n", W, H);
                      for (int i = 0; i < W * H; i++) { uint8_t p[3] = { fb[i], fb[i], fb[i] }; fwrite(p, 1, 3, fp); }
                      fclose(fp); }
            saved++; next_save += every;
        }
        if ((f % (int)fps) == (int)fps - 1) printf("t=%ds pc=%04X lines=%d\n", (int)(now + 1), ll_pc(), n);
    }
    if (wav) {
        uint32_t data = wav_samples * 2; uint8_t h[44];
        memcpy(h, "RIFF", 4); *(uint32_t *)(h + 4) = 36 + data; memcpy(h + 8, "WAVEfmt ", 8);
        *(uint32_t *)(h + 16) = 16; *(uint16_t *)(h + 20) = 1; *(uint16_t *)(h + 22) = 1; *(uint32_t *)(h + 24) = rate;
        *(uint32_t *)(h + 28) = rate * 2; *(uint16_t *)(h + 32) = 2; *(uint16_t *)(h + 34) = 16; memcpy(h + 36, "data", 4); *(uint32_t *)(h + 40) = data;
        fseek(wav, 0, SEEK_SET); fwrite(h, 1, 44, wav); fclose(wav);
    }
#ifdef LL_DEBUG
    { extern uint32_t ll_dbg_vggo, ll_dbg_pc_hist[0x10000]; printf("vggo %u\n", ll_dbg_vggo);
      for (int k = 0; k < 40; k++) { uint32_t best = 0; int bi = -1; for (int i = 0; i < 0x10000; i++) if (ll_dbg_pc_hist[i] > best) { best = ll_dbg_pc_hist[i]; bi = i; } if (bi < 0) break; printf("pc %04X: %u\n", bi, best); ll_dbg_pc_hist[bi] = 0; } }
#endif
    { extern uint32_t ll_dbg_snd_writes[256]; for (int v = 0; v < 256; v++) if (ll_dbg_snd_writes[v]) printf("snd %02X: %u  ", v, ll_dbg_snd_writes[v]); printf("\n"); }
    { const uint8_t *r = ll_ram(); printf("zp 00=%02X 73=%02X 74=%02X 82=%02X 83=%02X 84=%02X 85=%02X C1=%02X C2=%02X\n", r[0],r[0x73],r[0x74],r[0x82],r[0x83],r[0x84],r[0x85],r[0xC1],r[0xC2]); }
    printf("done: %u frames, %d images; vector bounding box x %d..%d  y %d..%d\n",
           ll_frame_count(), saved, gx0, gx1, gy0, gy1);
    return 0;
}
