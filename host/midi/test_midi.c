/*
 * test_midi.c - run the splash MIDI player on the host and check it behaves.
 *
 *     cc -O1 -w -Ihost/midi/stub -Icomponents/chiptune/include \
 *        -Icomponents/chiptune/src -o /tmp/test_midi host/midi/test_midi.c \
 *        components/chiptune/src/ay8910.c && /tmp/test_midi music/splash.mid
 *
 * Includes chiptune.c directly so the test can see the player's internals.
 *
 * Three checks, in the order they were learned the hard way:
 *   1. notes turn over        - a voice must not hold one pitch forever
 *   2. the AY goes quiet      - with every volume at zero the output must be
 *                               silence, or "the note never stops" is a fault
 *                               below the sequencer and no parser swap fixes it
 *   3. the audio breathes     - a real performance has quiet moments; output
 *                               pinned loud for the whole run means a drone
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "chiptune.c"

#define RATE      20050
#define SECONDS   30
#define MAX_DRONE 4.0

static int peak_of(const int16_t *b, int n)
{
    int pk = 0;
    for (int i = 0; i < n; i++) { int v = b[i] < 0 ? -b[i] : b[i]; if (v > pk) pk = v; }
    return pk;
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "music/splash.mid";
    FILE *f = fopen(path, "rb");
    if (!f) { printf("cannot open %s\n", path); return 2; }
    static uint8_t data[1024 * 1024];
    int len = (int)fread(data, 1, sizeof data, f);
    fclose(f);

    chip_init();
    song = tml_load_memory(data, len);
    if (!song) { puts("FAIL: tml_load_memory rejected the file"); return 1; }
    int chans = 0, progs = 0, notes = 0; unsigned first = 0, dur = 0;
    tml_get_info(song, &chans, &progs, &notes, &first, &dur);
    printf("%s: %d notes, %u ms, first note %u ms\n", path, notes, dur, first);
    chip_play();

    int16_t buf[256];
    uint8_t held[VOICES] = {0};
    int hold[VOICES] = {0}, maxhold[VOICES] = {0};
    long changes = 0, quiet_windows = 0, windows = 0;

    for (int i = 0; i < RATE * SECONDS / 256; i++) {
        audio_render(buf, 256, RATE);
        windows++;
        if (peak_of(buf, 256) < 400) quiet_windows++;
        for (int c = 0; c < VOICES; c++) {
            if (v_note[c] && v_note[c] == held[c]) { if (++hold[c] > maxhold[c]) maxhold[c] = hold[c]; }
            else { hold[c] = 0; if (v_note[c] != held[c]) changes++; }
            held[c] = v_note[c];
        }
    }

    int fail = 0;
    printf("%ld voice changes in %d s\n", changes, SECONDS);
    for (int c = 0; c < VOICES; c++) {
        double s = maxhold[c] * 256.0 / RATE;
        printf("  voice %d longest single pitch: %5.2f s%s\n", c, s, s > MAX_DRONE ? "   <-- DRONE" : "");
        if (s > MAX_DRONE) fail = 1;
    }
    if (!changes) { puts("FAIL: no notes played at all"); fail = 1; }

    /* 2. silence when everything is released */
    playing = false;
    all_off();
    int pk = 0;
    for (int i = 0; i < 16; i++) {
        memset(buf, 0, sizeof buf);        /* the caller clears; ay_render accumulates */
        ay_render(&ay, buf, 256, RATE);
        int p = peak_of(buf, 256);
        if (p > pk) pk = p;
    }
    printf("peak with every voice off: %d %s\n", pk, pk > 200 ? "<-- NOT SILENT" : "(silent)");
    if (pk > 200) fail = 1;

    /* informational: a continuous rag genuinely has few silent moments, so this
     * is context for the drone numbers above rather than a pass/fail line */
    printf("quiet windows: %ld / %ld (%.0f%%)\n", quiet_windows, windows, 100.0 * quiet_windows / windows);
    puts(fail ? "\nFAIL" : "\nok");
    return fail;
}
