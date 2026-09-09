/*
 * egg.h - the easter egg: play one MJPEG + MP3 clip from the "media" flash partition
 * over the game, then hand the screen and speaker back.
 */
#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Number of clips in the media partition (0 if it is missing or unreadable). */
int egg_clip_count(void);

/* Play clip `index` to the end (or until the fire button is pressed again). Blocks the
 * caller; the emulator must not be running meanwhile (the caller resets it afterwards). The render task must be idle
 * (render_wait_idle) because its frame buffer is borrowed for the decoders. Returns 0
 * on success. Audio goes through audio_hal's stream mode. */
int egg_play(int index);

/* Called once per player loop. Return true to stop the clip. The app runs its button and
 * tilt handling here, so the shared medal gestures keep working during playback. */
typedef bool (*egg_poll_fn)(void);
void egg_set_poll(egg_poll_fn fn);

#ifdef __cplusplus
}
#endif
