# MINIMAME

A multi-game arcade medal for the **Waveshare ESP32-C6-LCD-1.69**. Tilt to browse
marquees, hold the button to pick a game, and the medal boots straight into it
from then on.

MINIMAME itself is the menu. Each game is a separate firmware in its own slot,
and the launcher chain-boots them.

---

## Quick start

```sh
./minimame games      # what you can build, and which ROMs you already have
# put ROM zips in roms/  (see below)
./minimame build
./minimame flash
```

That is the whole thing. You do not edit a partition table, a header file or a
makefile. Everything is generated from the ROMs you supply.

---

## What you need to supply

**ROM zips.** We ship none, and finding them is your responsibility. Drop them in
`roms/` using their MAME names:

```
roms/galaga.zip
roms/mspacman.zip
roms/centiped.zip
```

`./minimame games` prints the approved list with a `*` next to the ones you have.
A game with no ROM is simply left out — no slot, no menu entry, no wasted flash.

**Nothing else.** The toolchain runs in Docker and the Python tools install
themselves into a local `.venv` on first build.

---

## Commands

| | |
|---|---|
| `./minimame` | what this build contains, and what it is missing |
| `./minimame games` | the approved list, and which ROMs you have |
| `./minimame build` | artwork → partition table → firmware |
| `./minimame flash [port]` | write everything to a connected medal |
| `./minimame clean` | remove build output |
| `./minimame help` | all of the above |

`./minimame` on its own changes nothing, so it is always safe to run when you want
to know where you stand.

---

## Choosing games

Presence of a ROM is the switch. That is usually all you want, but `games.toml`
can override it per game:

```toml
[[game]]
rom     = "gyruss"
title   = "Gyruss"
enabled = false      # never build it, even though roms/gyruss.zip exists
```

`enabled = true` forces a game in and fails loudly if its ROM is missing.

**Turning a game off frees its slot.** The partition table is generated, not
hand-maintained, so a five-game build gets five slots and the remaining flash
stays empty. This matters: ESP-IDF allows at most 16 app slots, and `configure.py`
will stop you before you exceed them rather than after.

Adding a game to the approved list means adding an entry to `games.toml` and
putting `marquees/<rom>.png` beside it. The build fails clearly if artwork is
missing.

---

## What actually happens on `build`

```
games.toml + roms/*.zip
   │
   ├─ tools/pack_marquees.py   fits artwork to 208×104, packs lcd/marquees.bin
   ├─ tools/configure.py       writes partitions.csv and build/manifest.json
   └─ docker idf.py build      compiles the launcher
```

Both generators read the same manifest, so the menu can never disagree with the
partition table about which games exist.

## Where the games live

Each game is its own project, and they sit in `games/`:

```
games/GIRDER/        Donkey Kong        games/SPINDLE/       Tempest
games/PELLETINO/     Ms. Pac-Man        games/TOCCATA/       Gyruss
games/VAUS/          Arkanoid           ...and so on
```

`games/medal-input/` sits alongside them: not a game, but the shared controls
component every one of them vendors a copy of.

Every one is a separate git repository with its own GitHub remote, so `games/`
is ignored by this repository rather than tracked as a pile of submodules. Clone
or move them freely; `tools/flash_all.sh` looks in `games/` by default and takes
`PROJECTS_DIR` if you keep them elsewhere.

`games.toml` names the project directory for each ROM, and flashing builds the
path from that plus the lowercased project name — `games/GIRDER/build_docker/girder.bin`.

Flashing writes the launcher, the artwork blob, and each game binary to the slot
labelled with its ROM name. Games you have not built yet leave their slot empty
and show as `NOT INSTALLED` in the menu — flash them later without rebuilding
anything else.

---

## Using it

| | |
|---|---|
| **Tilt left/right** | browse |
| **Hold the button 2 s** | pick that game — the medal now boots to it every time |
| **Hold the button 10 s in a game** | back to the menu |
| **Hold the button while powering on** | back to the menu, whatever is selected |

Taps do nothing, so a medal will not start itself in a pocket.

That last row is the one to remember. It always works, including when a selected
game is misbehaving.

---

## Troubleshooting

**`docker is installed but not running`** — start Docker Desktop.

**`nothing to build`** — `roms/` has no approved ROM zip. `./minimame games`.

**`N games enabled but ESP-IDF allows at most 16`** — disable some in
`games.toml`.

**`marquees/<rom>.png is missing`** — the game is on the approved list but has no
artwork. Add it, or set `enabled = false`.

**Menu says `NO ARTWORK`** — the `mqart` partition was never written. Re-run
`./minimame flash`.

**Every game says `NOT INSTALLED`** — expected before any game firmware is built.
The launcher works; the slots are empty.

**Tilt browses the wrong way** — flip the sign in `read_roll()` in
`main/input.cpp`.

---

## Documentation

- **`ARCHITECTURE.md`** — how the boot model works, what a game must do to be
  launchable, and the memory and byte-order rules. Read this before changing
  firmware or adding a game driver.
- **`HANDOFF.md`** — current state, what is verified and what is not, and the
  open task list.
- **`games.toml`** — the approved list, commented.

## Licensing

MINIMAME ships no ROMs and no game code. Marquee artwork is scanned cabinet art
belonging to its respective rights holders, included here for personal use on a
single device.
