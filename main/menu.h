#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* The rows the hold bar occupies, for a partial repaint while it fills. */
#define MENU_HOLD_BAR_Y0 206
#define MENU_HOLD_BAR_Y1 216

typedef enum {
    MENU_BROWSE = 0,     /* carousel, with the hold-to-pick progress bar */
    MENU_LAUNCHING,      /* the picked game's marquee while it boots */
    MENU_MESSAGE,        /* two lines of text, nothing else */
} menu_mode_t;

void        menu_init(void);
void        menu_set_mode(menu_mode_t m);
void        menu_nav(int delta);
void        menu_select_rom(const char *rom);   /* open the carousel on this game */
void        menu_render(void);
/* Repaint only the screen rows in [y0, y1). Bands that do not intersect are neither drawn
 * nor pushed, so animating something small does not wipe the whole panel. */
void        menu_render_range(int y0, int y1);
uint32_t    menu_last_render_us(void);   /* how long the last repaint took */
int         menu_band_rows(void);        /* rows composed before anything is pushed */
const char *menu_current_rom(void);
const char *menu_current_title(void);
void        menu_show_message(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif
