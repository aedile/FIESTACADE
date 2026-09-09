#include "render.h"
#include "esp_heap_caps.h"

#define SCRATCH_BYTES (96 * 1024)
static uint8_t *scratch;

uint8_t *render_scratch(size_t *size)
{
    if (!scratch) scratch = (uint8_t *)heap_caps_malloc(SCRATCH_BYTES, MALLOC_CAP_8BIT);
    *size = scratch ? SCRATCH_BYTES : 0;
    return scratch;
}
void render_wait_idle(void) { }
