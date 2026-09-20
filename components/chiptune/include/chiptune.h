/*
 * chiptune.h - plays a Standard MIDI File through an AY-3-8910.
 *
 * The repository ships no music. Drop a .mid in music/splash.mid and it is
 * embedded at build time; with no file the build still works and the splash
 * simply runs silent. Same arrangement as the ROMs and the marquee art: what
 * you play, and whether you are entitled to it, is between you and its owner.
 *
 * The AY core is the one the Moon Patrol, Time Pilot, Burger Time and Tapper
 * medals already emulate, so the only new code is the file parser.
 *
 * Three tone channels, so at most three notes sound at once; a fourth note
 * steals the quietest voice. Percussion (MIDI channel 10) is ignored.
 *
 * chiptune provides audio_render(), the hook audio_hal calls to fill its DMA
 * queue; call audio_update() regularly to keep it fed.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void chip_init(void);        /* set up the AY and parse the file; after audio_init() */
bool chip_has_music(void);   /* false when no MIDI was embedded, or it would not parse */
void chip_play(void);        /* start (or restart) from the top */
void chip_stop(void);
void chip_free(void);    /* release the parsed song (frees a fair chunk of heap) */        /* silence all three channels */
bool chip_playing(void);

#ifdef __cplusplus
}
#endif
