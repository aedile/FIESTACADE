// Generated. Marquees fitted inside 208x104, RGB565.
#pragma once
#include <stdint.h>

#define MARQUEE_BOX_W 208
#define MARQUEE_BOX_H 104
#define MARQUEE_COUNT 16

typedef struct {
    const char     *rom;
    const char     *title;
    uint16_t        w, h;
    const uint16_t *data;
} marquee_t;

extern const uint16_t marquee_arkanoidu_data[16640];
extern const uint16_t marquee_asteroid_data[12688];
extern const uint16_t marquee_centiped_data[11648];
extern const uint16_t marquee_digdug_data[12064];
extern const uint16_t marquee_dkong_data[11440];
extern const uint16_t marquee_esb_data[10192];
extern const uint16_t marquee_frogger_data[11232];
extern const uint16_t marquee_galaga_data[16848];
extern const uint16_t marquee_gyruss_data[14976];
extern const uint16_t marquee_missile_data[12064];
extern const uint16_t marquee_mspacman_data[14768];
extern const uint16_t marquee_pacman_data[16016];
extern const uint16_t marquee_polepos_data[12272];
extern const uint16_t marquee_rallyx_data[16848];
extern const uint16_t marquee_starwars_data[18616];
extern const uint16_t marquee_tempest_data[14768];

extern const marquee_t marquees[MARQUEE_COUNT];
