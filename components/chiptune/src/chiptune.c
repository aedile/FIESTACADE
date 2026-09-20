/*
 * chiptune.c - plays a Standard MIDI File on an AY-3-8910.
 *
 * File parsing and timing are TinyMidiLoader's (tml.h, zlib, Bernhard
 * Schelling). It hands back a flat list of events already stamped in absolute
 * milliseconds, so there is no tick arithmetic, no tempo tracking and no
 * running-status handling here - which is exactly where a hand-rolled parser
 * goes wrong.
 *
 * What is left is the part specific to this hardware: mapping a general MIDI
 * stream onto three square-wave tone channels.
 */
#include "chiptune.h"
#include "ay8910.h"
#include "audio_hal.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

#define TML_NO_STDIO
#define TML_IMPLEMENTATION
#include "tml.h"

static const char *TAG = "chiptune";

#define AY_CLOCK  1789773u
#define VOICES    3
#define DRUM_CH   9                    /* MIDI channel 10, zero-based */

#ifdef HAVE_SPLASH_MIDI
extern const uint8_t midi_start[] asm("_binary_splash_mid_start");
extern const uint8_t midi_end[]   asm("_binary_splash_mid_end");
#endif

static ay8910_t     ay;
static tml_message *song;              /* head of the list, kept for tml_free */
static tml_message *cur;               /* next event to play */
static uint64_t     samples_played;
static bool         playing;

static uint8_t v_note[VOICES], v_vol[VOICES];

/* ---- AY ------------------------------------------------------------------- */

static void wr(uint8_t reg, uint8_t val) { ay_address_w(&ay, reg); ay_data_w(&ay, val); }

/* period = clock / (16 * freq); table is C7..B7 (MIDI 96..107), shifted per octave */
static const uint16_t oct[12] = { 53, 50, 48, 45, 42, 40, 38, 36, 34, 32, 30, 28 };
static uint16_t period_for(uint8_t n)
{
    if (n < 24 || n > 107) return 0;
    return (uint16_t)(oct[n % 12] << (8 - n / 12));
}

static void voice_set(int c, uint8_t note, uint8_t vol)
{
    uint16_t p = period_for(note);
    wr((uint8_t)(c * 2), (uint8_t)(p & 0xFF));
    wr((uint8_t)(c * 2 + 1), (uint8_t)(p >> 8));
    wr((uint8_t)(8 + c), (uint8_t)(vol & 0x0F));   /* bit 4 would hand the channel
                                                    * to the envelope generator */
    v_note[c] = vol ? note : 0;
    v_vol[c]  = vol;
}

static void all_off(void) { for (int i = 0; i < VOICES; i++) voice_set(i, 0, 0); }

static void note_on(uint8_t note, uint8_t vel)
{
    if (!period_for(note)) return;
    /* retrigger in place: two voices on one pitch is how notes get stranded,
     * because a note-off only ever frees one of them */
    for (int i = 0; i < VOICES; i++)
        if (v_note[i] == note) { voice_set(i, note, (uint8_t)(4 + vel * 11 / 127)); return; }

    int c = -1;
    for (int i = 0; i < VOICES; i++) if (!v_note[i]) { c = i; break; }
    if (c < 0) {
        /* All busy. Keep the outer voices - melody on top, bass at the bottom -
         * and only displace the middle one, and only for something above it. */
        int low = 0, high = 0;
        for (int i = 1; i < VOICES; i++) {
            if (v_note[i] < v_note[low])  low  = i;
            if (v_note[i] > v_note[high]) high = i;
        }
        int mid = (low == high) ? 0 : 3 - low - high;
        if (note <= v_note[mid]) return;
        c = mid;
    }
    voice_set(c, note, (uint8_t)(4 + vel * 11 / 127));
}

static void note_off(uint8_t note)
{
    for (int i = 0; i < VOICES; i++) if (v_note[i] == note) voice_set(i, 0, 0);
}

/* ---- public --------------------------------------------------------------- */

void chip_init(void)
{
    ay_init(&ay, AY_CLOCK, NULL, NULL);
    ay_reset(&ay);
    wr(7, 0x38);                       /* tones A/B/C on, noise off */
    all_off();

#ifdef HAVE_SPLASH_MIDI
    if (!song) {
        /* tml_load_memory dereferences its own failed allocations, so check for
         * room first rather than fault inside the library. The event list runs to
         * roughly eight times the file on a dense score. */
        size_t need = (size_t)(midi_end - midi_start) * 10;
        size_t have = heap_caps_get_free_size(MALLOC_CAP_8BIT);
        if (have < need) {
            ESP_LOGW(TAG, "only %u bytes free, need about %u for this file: skipping music",
                     (unsigned)have, (unsigned)need);
            return;
        }
        song = tml_load_memory(midi_start, (int)(midi_end - midi_start));
        if (!song) { ESP_LOGW(TAG, "music/splash.mid did not parse; splash will be silent"); return; }
        int chans = 0, progs = 0, notes = 0;
        unsigned int first = 0, len = 0;
        tml_get_info(song, &chans, &progs, &notes, &first, &len);
        ESP_LOGI(TAG, "MIDI: %d notes, %u ms long, first note at %u ms", notes, len, first);
    }
#else
    ESP_LOGI(TAG, "no music/splash.mid embedded; splash will be silent");
#endif
}

bool chip_has_music(void) { return song != NULL; }

void chip_play(void)
{
    if (!song) return;
    cur = song;
    samples_played = 0;
    all_off();
    playing = true;
}

void chip_stop(void)
{
    playing = false;
    all_off();
}

void chip_free(void)
{
    chip_stop();
    if (song) { tml_free(song); song = NULL; cur = NULL; }
}

bool chip_playing(void) { return playing; }

void audio_render(int16_t *buf, int samples, int rate)
{
    if (!playing || !song) { memset(buf, 0, (size_t)samples * sizeof(int16_t)); return; }

    while (samples > 0) {
        unsigned int now_ms = (unsigned int)(samples_played * 1000ull / (unsigned)rate);

        /* everything due by now */
        while (cur && cur->time <= now_ms) {
            if (cur->channel != DRUM_CH) {
                if (cur->type == TML_NOTE_ON && cur->velocity > 0)
                    note_on((uint8_t)cur->key, (uint8_t)cur->velocity);
                else if (cur->type == TML_NOTE_OFF ||
                         (cur->type == TML_NOTE_ON && cur->velocity == 0))
                    note_off((uint8_t)cur->key);
            }
            cur = cur->next;
        }
        if (!cur) { chip_play(); continue; }        /* loop back to the top */

        /* render up to the next event, so notes land on the right sample */
        uint64_t due = (uint64_t)cur->time * (unsigned)rate / 1000ull;
        int n = (due > samples_played) ? (int)(due - samples_played) : 1;
        if (n > samples) n = samples;

        /* ay_render ACCUMULATES into the buffer so several chips can be summed,
         * and it returns without touching it at all when every channel is
         * silent. Both mean the caller owns clearing it - miss that and a silent
         * passage replays whatever was in the DMA buffer last, forever. */
        memset(buf, 0, (size_t)n * sizeof(int16_t));
        ay_render(&ay, buf, n, rate);
        buf += n; samples -= n; samples_played += (unsigned)n;
    }
}
