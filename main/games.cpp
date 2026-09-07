/*
 * games.cpp - chain-booting a game image.
 *
 * Every game gets an app partition LABELLED WITH ITS ROM NAME (see partitions.csv),
 * so the launcher needs no table mapping games to slots: it asks flash whether a
 * partition by that name exists. A game that has never been flashed simply has no
 * partition, and the menu greys it out. Adding a game means adding a marquee and a
 * partition - the launcher never gets recompiled.
 *
 * Which game is selected, and the handshake every game owes the menu, live in
 * components/medalboot - shared so the games can use the same API.
 */
#include "games.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "games";

static const esp_partition_t *find_game(const char *rom)
{
    if (!rom || !*rom) return NULL;
    return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, rom);
}

bool game_installed(const char *rom)
{
    return find_game(rom) != NULL;
}

bool game_launch(const char *rom)
{
    const esp_partition_t *p = find_game(rom);
    if (!p) {
        ESP_LOGW(TAG, "%s is not installed", rom);
        return false;
    }

    esp_err_t err = esp_ota_set_boot_partition(p);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not select %s: %s", rom, esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "booting %s at 0x%06x", rom, (unsigned)p->address);
    esp_restart();
    return true;   /* unreachable */
}
