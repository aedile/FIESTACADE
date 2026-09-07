#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A game is installed iff an app partition is labelled with its ROM name. */
bool game_installed(const char *rom);
/* Chain-boots the game. Does not return on success. */
bool game_launch(const char *rom);
#ifdef __cplusplus
}
#endif
