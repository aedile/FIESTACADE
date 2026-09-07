# MINIMAME

A menu medal. It browses marquee artwork and chain-boots the other game medals;
it runs no emulation itself.

Hardware: Waveshare ESP32-C6-LCD-1.69 — 240×280 ST7789V2, 16 MB flash,
512 KB SRAM, **no PSRAM**. QMI8658 IMU, BOOT (GPIO9) and PWR (GPIO18) buttons.

## Why chain-boot instead of one image

Six game firmwares already exist as independent projects, and their `display.cpp`
has diverged between them. Merging them into shared components would be a large
refactor with real regression risk on games that currently work, to reclaim about
3 MB of duplicated IDF runtime out of 16 MB that is otherwise unused.

Flash is the cheap resource here. Each game keeps its own repo and its own image.

## The partition trick

Every game's app partition is **labelled with its ROM name**, so the launcher
needs no table mapping games to slots:

```c
esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "galaga")
```

`NULL` means not installed, and the menu greys that marquee out. Adding a game
means adding a marquee and a partition — **the launcher is never recompiled**.

## Sticky selection

The medal remembers what you picked. Once a game is selected it boots straight
into it and the menu never appears, which is what you want on a medal you hand to
someone. The selection lives in NVS and survives power cycles.

| | |
|---|---|
| In the menu, **hold the button 2 s** on a game | selects it and boots it, permanently |
| In a game, **hold the button 10 s** | forgets the selection, returns to the menu |
| **Hold the button while powering on** | forgets the selection, shows the menu |

Taps do nothing at all, so a medal cannot be started by a knock in a pocket. The
menu fills a progress bar while the button is held — without it nobody knows how
long "a few seconds" means.

The power-on escape is the one that always works, and it is worth knowing about
before you need it.

## What a game must do

Three lines, all from `components/medalboot` (copy it into the game project or
point `EXTRA_COMPONENT_DIRS` at it, so both ends agree on the NVS keys):

```c
void app_main(void)
{
    medalboot_game_startup();          // FIRST LINE, before anything risky
    ...init...
    medalboot_game_running();          // once stable, a few seconds in

    for (;;) {
        if (medalboot_exit_hold(button_is_down)) medalboot_exit_to_menu();
        ...
    }
}
```

`medalboot_game_startup()` points the boot partition back at the launcher before
anything can fail, so a panic, a watchdog bite or a brownout lands in the menu
rather than boot-looping a broken game. `medalboot_game_running()` clears the
loop guard: if a selected game is booted `MEDALBOOT_MAX_ATTEMPTS` times without
ever confirming it got that far, the launcher gives up on it and shows the menu
instead of retrying forever.

A game that carries more than one ROM set reads `medalboot_rom()` to find out
which one it was asked for.

## Memory

There is no framebuffer. 240×280 RGB565 would be 131 KB of 512 KB with no PSRAM,
so the screen is drawn in horizontal bands (`components/gfx`): clear a band, draw
what intersects it, push it, move down. Two buffers are the whole UI cost — a
19 KB band and a 17 KB scratch for marquee rows.

**Every pixel in a band is RGB565 stored big-endian.** The marquee blob is packed
that way too, so blitting art is a `memcpy` rather than a per-pixel byte swap, and
bands go straight to `display_write_preswapped()`. Build colours with `gfx_rgb()`,
which returns a value already swapped — never write raw RGB565 into a band.

Marquees are read with `esp_partition_read()` rather than a memory map. That lands
pixels in RAM, which SPI DMA can reach — **flash-mapped addresses cannot be used as
a DMA source** — and avoids the MMU's limits on how much can be mapped at once.

## Input

Tilt browses, one game per detent. Raw tilt would rip through sixteen games in
half a second, so a step fires only when roll crosses 18°, and no further step can
fire until roll falls back inside 8°. Holding past the threshold auto-repeats,
slowly at first. PWR short-presses re-level the neutral pose; long-press cuts the
battery rail. `BAT_EN` (GPIO15) must stay HIGH or the medal powers itself off.

## Artwork pipeline

```
marquees/*.png            source art, any resolution
  └─ tools/pack_marquees.py
       ├─ lcd/preview/*.png   fitted PNGs, for eyeballing
       └─ lcd/marquees.bin    the blob that gets flashed
            └─ tools/flash_mqart.sh   writes it to the mqart partition
```

Art is fitted inside 208×104 preserving aspect. At 208 wide, most marquees land
between 49 and 81 px tall; Star Wars is the one outlier that is height-limited
instead, at 179×104. The app and the artwork flash separately, so updating every
marquee needs no rebuild.

## Flash budget (16 MB)

| Region | Size | Notes |
|---|---|---|
| `launcher` | 512 KB | built: 238 KB, 53% free |
| `mqart` | 640 KB | blob: 436 KB, room for ~6 more games |
| 16 game slots | ~12.3 MB | right-sized per build, not uniform |
| free | ~2.25 MB | |

**All 16 OTA slots are used — `ota_0` through `ota_15` is ESP-IDF's hard cap.**
If a seventeenth game is ever wanted, collapse a pair that shares hardware onto
one image and select the ROM at boot: PELLETINO already carries Pac-Man and
Ms. Pac-Man, and WALKERRUN carries Star Wars and Empire Strikes Back. Each merge
frees a slot and deletes a duplicated codebase.

## Build

```sh
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
    idf.py -B build_docker -DIDF_TARGET=esp32c6 build
```

Flashing must happen from the host — Docker Desktop on macOS cannot reach USB.
