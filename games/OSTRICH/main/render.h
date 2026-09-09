#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void render_init(void);
uint8_t *render_acquire(void);           /* a free snapshot buffer of JO_SNAP_SIZE bytes, or NULL */
void render_submit(uint8_t *snap);       /* hand a filled one over; the palette is read now */
uint32_t render_frames_drawn(void);
uint32_t render_frames_dropped(void);
uint64_t render_busy_us(void);
#ifdef __cplusplus
}
#endif
