#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Plays the opening sequence with music. Blocking, about 6 seconds; any press of
 * the button ends it early. Safe to call when audio or the framebuffer will not
 * initialise - it simply returns. */
void splash_run(void);

#ifdef __cplusplus
}
#endif
