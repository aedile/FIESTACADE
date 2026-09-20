#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Battery rail, gauge and over-discharge protection.
 *
 * This module owns BAT_EN (GPIO15). The rail must be held high or the medal
 * browns out, and cutting it is how the medal switches itself off. */

#define BATT_LOW_PCT   15        /* meter turns amber */
#define BATT_CRIT_PCT   5        /* meter turns red */

void battery_init(void);
int  battery_percent(void);      /* 0..100, cached, re-read every few seconds */
int  battery_mv(void);           /* last reading in mV, 0 before the first */
bool battery_present(void);      /* false when the ADC would not start */

/* Call regularly. Cuts power if the pack has been under the floor for long
 * enough to rule out a transient sag - lithium cells are damaged by sitting
 * below ~3.0 V, and the medal has no protection board of its own. */
void battery_tick(void);

void battery_power_off(void);    /* blank the panel, drop the rail, halt */

#ifdef __cplusplus
}
#endif
