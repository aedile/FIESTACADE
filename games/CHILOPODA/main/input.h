#pragma once
#include <stdint.h>
#include "centiped.h"
#ifdef __cplusplus
extern "C" {
#endif
void input_init(void);
void input_update(ce_input_t *in);
#ifdef __cplusplus
}
#endif
