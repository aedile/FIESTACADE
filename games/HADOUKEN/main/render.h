#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The player borrows a scratch buffer for its decoders. There is no game here, so this is
 * simply a buffer of the size the player asks for. */
uint8_t *render_scratch(size_t *size);
void     render_wait_idle(void);
#ifdef __cplusplus
}
#endif
