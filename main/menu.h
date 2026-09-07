#pragma once
#ifdef __cplusplus
extern "C" {
#endif

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
const char *menu_current_rom(void);
const char *menu_current_title(void);
void        menu_show_message(const char *line1, const char *line2);

#ifdef __cplusplus
}
#endif
