# FIESTACADE

A multi-game arcade medal for the **Waveshare ESP32-C6-LCD-1.69**. Tilt the medal
to browse marquees, hold the button to pick a game, and it boots straight into
that game from then on — until you deliberately come back to the menu.

FIESTACADE itself is the menu (the "launcher"). Each game is a separate firmware
image in its own flash slot; the launcher chain-boots them. Everything in a build
— the partition table, the artwork, the menu — is generated from the ROMs you
supply. You never hand-edit a partition table, a header, or a makefile.

> **ROMs are yours to supply.** This project ships none. Drop the zips you own
> into `roms/` using their MAME names. Marquee artwork is copyrighted too, so we
> ship none of that either — see [Marquee art](#marquee-art).

---

## Quick start

```sh
./fiestacade games      # the approved list, and which ROMs you already have
# put ROM zips in roms/ using their MAME names
./fiestacade pick       # (only if more games are present than fit) choose a build
./fiestacade build      # artwork + partition table + launcher firmware
./fiestacade flash      # write it all to a connected medal
```

`./fiestacade` on its own reports what the current build contains and what it is
missing. `./fiestacade help` lists every command.

A game appears in the menu only if its ROM is in `roms/`. Games you don't have a
ROM for are simply absent — no slot, no menu entry, no wasted flash.

---

## Choosing a build — `fiestacade pick`

The medal has room for **16 game slots** inside **16 MB** of flash, shared with
the launcher and the artwork. More games are approved than fit, and you may hold
more ROMs than fit. When a build overflows, `./fiestacade pick` walks the list of
everything you have and prices each choice live — slots used, flash used, space
free — so you can land a build that fits before you flash it:

```
   1. [x] Ms. Pac-Man          1024 KB   mspacman
   2. [x] Pole Position        1024 KB   polepos
   3. [ ] Star Wars             768 KB   starwars
   ...
  slots 11/16   flash 12.75/16 MB   3.25 MB free
```

Type a number to toggle a game, `a` to auto-pick everything that fits, `s` to
save, `q` to quit. It writes `selection.txt`, which the build honours. Delete
that file (or `./fiestacade pick --clear`) to go back to "every ROM present is
included." Picking is entirely optional — reach for it only when a build overflows.

---

## Controls

Every game is played the same way physically: **hold the medal upright and twist
or tip it** — the tilt sensor is the joystick, spinner, wheel, or yoke. Two
buttons do the rest.

| Button | Short press | Hold |
|---|---|---|
| **BOOT** (top) | the game's action — fire / jump / hop / pump | 3 s: sound off/on · 10 s: back to the menu |
| **PWR** (side) | insert a coin (then auto-start ½ s later) | 1 s: power the medal off |

> A **coin** is always a coin and a **start** is always a start, on every game.
> Sound-off (mute) and back-to-menu are the same gesture everywhere. Picking a
> game from the menu is a button *hold*; a knock won't do it.

Per-game, the tilt and the BOOT action are:

| Game | ROM | Tilt does | BOOT does |
|---|---|---|---|
| Pac-Man | `pacman` | steer (4-way maze) | — |
| Ms. Pac-Man | `mspacman` | steer (4-way maze) | — |
| Galaga | `galaga` | move the fighter L/R | fire |
| Galaxian | `galaxian` | move the fighter L/R | fire |
| Space Invaders | `invaders` | move the cannon L/R | fire |
| Lunar Lander | `llander` | twist = rotate, tip away = throttle (analogue) | abort |
| Dig Dug | `digdug` | move L/R (dominant axis) | pump |
| Mr. Do! | `mrdo` | dig in 4 directions (dominant axis) | throw the power ball |
| Burger Time | `btime` | walk in 4 directions (dominant axis) | pepper |
| Root Beer Tapper | `rbtapper` | twist along the bar, tip between bars | pour (hold to fill) |
| Donkey Kong | `dkong` | run / climb (dominant axis) | jump |
| Mario Bros. | `mario` | run L/R | jump |
| Frogger | `frogger` | hop L/R (dominant axis) | hop forward |
| Rally-X | `rallyx` | drive L/R | lay a smoke screen |
| Centipede | `centiped` | trackball L/R (angle = speed) | fire |
| Missile Command | `missile` | trackball L/R (angle = speed) | fire (cycles the 3 bases) |
| Asteroids | `asteroid` | twist = rotate, tip away = thrust | fire (hold 0.7 s: hyperspace) |
| Tempest | `tempest` | claw around the rim | fire (short 2nd press: superzapper) |
| Gyruss | `gyruss` | move around the ring | fire |
| Time Pilot | `timeplt` | 8-way: twist and tip to point the plane | fire |
| Arkanoid | `arkanoidu` | paddle, absolute (±32° sweep) | fire (once the laser is fitted) |
| Star Wars | `starwars` | flight yoke — twist yaws, tip pitches | fire (also starts, in free play) |
| Pole Position | `polepos` | steer like a wheel | — (throttle is automatic; BOOT taps shift gear) |
| Street Fighter II | `sf2` | — (attract-mode video, see below) | — |

Pole Position starts on a coin (free-play) and holds the accelerator down for
you, so the whole game is one wheel plus the gear tap — every other gesture then
matches the rest of the medal.

---

## Adding a video "game"

A game slot can hold a looping video clip instead of an emulator — Street Fighter
II ships as its attract-mode reel. The clip lives in a data partition of its own.

- Encode and pack a clip with `games/HADOUKEN/tools/pack_media.py` — it letterboxes
  to the portrait panel and writes `media.bin`. See that script for the size limit
  and encoding settings; a longer clip needs a bigger `data_kb` in `games.toml`.
- The video slot is switched on by `games/HADOUKEN/media.bin` existing, the same
  way a ROM zip switches on an emulated game.

Only one game per build may carry a data partition (it is labelled `media`, which
is the label the player looks for).

---

## Marquee art

The carousel shows a marquee for each game. **Game logos are copyrighted, so we
ship none.** You have two options:

- **Fetch:** `./fiestacade art` pulls marquees from a third-party archive into
  `marquees/`. See `tools/fetch_marquees.py` for the source and the `FIESTACADE_ART_BASE`
  override.
- **Supply your own:** drop a PNG named `marquees/<rom>.png` (any resolution; it's
  fitted to a 208×104 box).

Any game without a marquee gets a plain generated text banner, so the build never
blocks on missing art.

---

## Two games, one slot

Pac-Man and Ms. Pac-Man are carried in **one image** that picks which to run at
boot, so the pair costs one slot instead of two. In `games.toml`, Ms. Pac-Man owns
the slot and Pac-Man rides it (`boots = "mspacman"`); both still appear as separate
menu entries, and selecting either records which one the shared image should run.
The same mechanism (`boots = "<owner>"`) works for any two games that share an image.

---

## How a build is laid out

```
nvs / otadata / phy_init         housekeeping
launcher            0x20000      the FIESTACADE menu (factory app)
mqart                            marquee artwork blob (lcd/marquees.bin)
ota_0..ota_N                     one app slot per game, labelled with its ROM
media               (optional)   a video clip's data partition
```

`tools/configure.py` generates `partitions.csv` and `build/manifest.json` from
`games.toml` plus whatever is in `roms/`, so the firmware and the flasher can
never disagree about where a game lives. The launcher finds a game by its
partition label, so adding a game never recompiles the launcher.

---

## Repository layout

- `games/` — every game's source, vendored in so one clone builds everything. Each
  folder is also mirrored at its own `aedile/*` GitHub repo; `VENDORED.md` records
  the upstream and commit for each.
- `components/` — code shared by the launcher: display, gfx, IMU, `mqart` (artwork),
  `medalboot` (which game boots, and the way back out), `medal_input` (buttons,
  battery, coin/start, mute/exit holds, the tilt zero).
- `tools/` — the build tooling behind `./fiestacade`.
- `games.toml` — the one place that decides what a build *can* contain.

---

## Building without Docker

`./fiestacade build` uses the `espressif/idf:v5.3.4` Docker image so you need
nothing installed but Docker. If you have ESP-IDF v5.3.4 natively, you can run the
underlying steps yourself; see the commands `./fiestacade` prints.

Docker Desktop on macOS can't reach USB, so **flashing always runs on the host** —
that is what `tools/flash_all.sh` (behind `./fiestacade flash`) does.
