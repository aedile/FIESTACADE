# MINIMAME — handoff

> ## Status, 6 September 2026 — it has now run on hardware
>
> The launcher boots on a medal, finds its artwork and reports **"14 games in the
> carousel"**. Everything below this box was written when nothing had ever been
> flashed; the task list is kept for its reasoning, but read this first.
>
> **Done since:**
>
> - Every one of the fourteen games has a driver, a measured slot size and a build.
>   `games.toml` records the project for each; nothing is a placeholder any more.
> - All fourteen game repos are on `components/medalboot`:
>   `medalboot_game_startup()` is the genuine first line of `app_main`,
>   `medalboot_game_running()` runs once past init, and holding the button for ten
>   seconds leaves for the menu. The old per-repo `launcher_handback.h` is gone.
> - The whole medal has been flashed: bootloader, partition table, launcher,
>   artwork and all fourteen images, 9.69 MB of 16. Every write verified.
> - Two new drivers joined the set: **Tempest** (`SPINDLE`) and **Gyruss**
>   (`TOCCATA`).
>
> **Two things found by running it:**
>
> - The launcher zeroed its browse axis at power-on, with the medal flat on a
>   desk. Held flat, gravity points out of the screen and the in-plane angle is
>   noise, so the carousel's centre was a pose nobody was holding. `read_roll()`
>   now refuses a reading it cannot trust and `capture_neutral()` retries until
>   the medal is picked up. The same fault was fixed in all fourteen games.
> - **`CONFIG_PM_ENABLE` silences the console.** With dynamic frequency scaling on,
>   nothing comes out of the USB serial port once the app starts. The launcher is
>   running fine — build with `CONFIG_PM_ENABLE=n` and the log comes back. Worth
>   knowing before assuming a silent medal is a dead one.
>
> **The menu has now been looked at**, on hardware and in a host preview
> (`host/preview`, which compiles the real `menu.cpp` against the real blob).
> Jesse's report was "weird lines in the menu". It was the font: the glyph
> outlines are scaled but the pen was always one pixel, so at double size the
> letters came out spindly and their stair-stepped diagonals read as rows of
> disconnected dashes. The pen now scales with the text. Ruled out first, and
> worth not re-deriving: the artwork renders byte-identical to the blob, and the
> band compositor is exact (`-DGFX_BAND_H=280` gives byte-identical screens).
>
> "Missile Command" is 238 of the 240 columns at double size, so a title that
> would touch the edges now drops to single size. One of the fourteen does.
>
> **Felt on hardware, 7 September:** "I still see bands" and "the scrolling
> needs to be gentler, requires too much precision."
>
> The bands were not the compositor. While the button was held, the main loop
> repainted the *entire* screen — all seven bands, with the marquee re-read from
> flash for three of them — twenty times a second, to animate a five-pixel
> progress bar. SPI is already at the ST7789's 80 MHz ceiling, so the fix was to
> stop repainting what had not changed: `menu_render_range()` skips bands that do
> not intersect, and the bar animation now touches one band instead of seven.
> `host/preview` checks a partial repaint lands pixel-for-pixel what a full one
> would.
>
> The detent went from 18°/8° to 12°/6°, and the auto-repeat now winds up the
> longer it is held — a flick still moves one game, leaning on it crosses all
> fourteen in 1.6 s rather than 3.3 s. A fixed repeat rate is either too fast to
> land on a game or too slow to cross the menu; there is no single rate that is
> both, which is why it ramps.
>
> **The actual cause of the bands, found on the third pass, and the two
> diagnoses before it were wrong.** `display_write_preswapped()` silently
> truncated anything larger than one DMA buffer - 7168 bytes, about fifteen
> rows - and dropped the rest. The launcher pushed 40-row bands of 19200 bytes,
> so from the very first flash every band painted its first fifteen rows and
> left the other twenty-five stale. That is the horizontal banding. It was never
> the font, the progress bar, or the strip-by-strip stall; those were real but
> beside the point. Composing the whole screen and pushing it once then painted
> only the top fifteen rows of the entire panel - the "black with a lighter band
> on top" that finally gave it away. The driver now sends any length in
> DMA-sized pieces, double-buffered. The games never hit this because they push
> fourteen rows at a time.
>
> Why the host preview could not see it: its panel stub accepted unlimited
> writes. It now models the DMA limit, and `-DPREVIEW_TRUNCATING_DRIVER` (with
> `-DPREVIEW_STRIPS` for the original layout) reproduces both symptoms exactly
> before showing the fix. **A preview that stubs the hardware is only as honest
> as the stub** - it verified pixels perfectly for two days while the panel path
> threw most of them away.
>
> **Measured on hardware, second pass.** The first banding fix was not enough
> because it only covered the progress bar. The scroll repaint was the bigger
> one: every step drew, pushed and waited seven times, so the panel showed new
> content above the seam and old below while it walked down. The launcher runs no
> emulator and had 257 KB free, so it now composes the whole screen and pushes
> once - **one 10.4 ms sweep instead of seven stalled strips**. It falls back to
> strips if the 134 KB DMA buffer is ever unavailable, and says which it got at
> boot.
>
> **Still open:**
>
> - The detent has been retuned twice against feel. It is now 12 degrees out, 6
>   back, a one-second dwell before the auto-repeat starts, and a wind-up to 170
>   ms rather than 65. Not felt yet.
> - Chain-booting a game from the menu has not been exercised on hardware.
> - Pac-Man is disabled in `games.toml`. PELLETINO carries both it and Ms. Pac-Man
>   but which one is a build-time choice, so the two would need two builds of one
>   repo and the flashing script expects one binary per project. Merging them into
>   a single image that reads `medalboot_rom()` at boot is the fix, and would free
>   a slot as well.
> - ~~Star Wars has no in-game exit gesture.~~ **Wrong, and it was my error.** The
>   easter egg was supposed to have been removed (task 2 below) and had not been,
>   so its thirteen-second hold was still occupying the button. The egg is gone
>   now - along with egg.cpp, the helix MP3 decoder nothing else used, 9 MB of
>   clips and a 15 MB media partition - and Star Wars has the same fire / 3 s
>   sound / 10 s exit as every other medal. The image lost 55 KB with it.


You are picking up a launcher medal that browses arcade marquees and chain-boots
other game images on a Waveshare ESP32-C6-LCD-1.69. The launcher is written and
builds clean. **It has never run on hardware.** Nobody has a device tonight, so
every task below is one you can finish and prove with a build, a byte comparison
or a size check — not with your eyes on a panel.

Read `ARCHITECTURE.md` first. It explains the boot model and is the contract the
games have to satisfy. This document is only what to do next.

---

## This is the flagship project — ease of use is a feature, not polish

MINIMAME is the one people will actually pick up. **Documentation and
command-line ergonomics are a first-class requirement, ranked alongside working
firmware — not something to do at the end if there is time.**

The bar: someone who has found their own ROMs should be able to go from a fresh
clone to a flashed medal without reading source, editing a header, or learning
what a partition table is. Everything else is generated.

```sh
./minimame games      # what can I build, what do I have?
./minimame build
./minimame flash
```

Concretely, this means:

- **Every failure prints what to do next**, not just what went wrong. Compare
  `error: nothing to build - put an approved ROM zip in roms/ (see games.toml)`
  against a Python traceback. Match the former.
- **Never require a hand edit that a script could make.** Adding a game is a zip
  in `roms/`. It is not editing `partitions.csv` — that file is generated and
  says so at the top.
- **`./minimame` with no arguments must be safe and informative.** It reports and
  changes nothing.
- **If you add a command or a flag, it goes in `README.md` and `./minimame help`
  in the same commit.** Undocumented capability is not capability.
- **If you change behaviour described in `README.md` or `ARCHITECTURE.md`, update
  them in that commit.** A doc that lies is worse than no doc, and this one is
  aimed at people who did not write the code.

Read `README.md` before you start. If any instruction in it is wrong or no longer
works, fixing that is higher priority than any feature below.

---

## Ground truth

**Verified.** Do not re-litigate these; they were checked, not assumed.

- `minimame.bin` builds clean under `espressif/idf:v5.3.4`, 246 KB in its 512 KB
  slot, no warnings from MINIMAME sources.
- `tools/pack_marquees.py` reproduces `lcd/marquees.bin` byte-for-byte from the
  source art.
- All 16 blob entries are in-bounds with `len == w*h*2`; magic is `MQ02`.
- `partitions.csv` is now **generated** by `tools/configure.py` from `games.toml`
  plus whatever is in `roms/`; it validates alignment, overlap, the 16-slot OTA
  cap and the flash budget, and refuses to write a table that breaks any of them.
- `tools/flash_all.sh --dry-run` resolves real offsets and real game binary paths.

**Unverified. Assume nothing here is right.**

- Nothing has been flashed or run. Not the menu, not the boot logic, not one pixel.
- IMU roll direction. If tilting right walks the carousel left, flip the sign in
  `read_roll()` in `main/input.cpp`.
- Detent constants (18° to step, 8° to re-arm, 450 ms to repeat) and the 2 s
  select hold. Numbers chosen by reasoning, never felt in a hand.
- The whole screen layout. The 208×104 box, 20 px title, dot row and progress bar
  are arithmetic. No one has seen them rendered.
- Auto-boot, the attempts counter and the power-on escape have never executed.

---

## Environment

There is no local ESP-IDF and no `idf.py` on the host. Everything builds in Docker,
which is running and already has the image pulled.

```sh
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
    idf.py -B build_docker -DIDF_TARGET=esp32c6 build
```

Python for the asset tools needs a venv — the system Python is externally managed
(PEP 668) and `pip install` will refuse. Do not pass `--break-system-packages`.

Flashing must happen from the host, never the container: Docker Desktop on macOS
cannot reach USB devices. You will not be flashing tonight anyway.

---

## Task list, in order

### 0. Get the working games' ROMs into `roms/` — P0, do this first

`./minimame games` currently shows a mess: the nine games with **no driver** have
ROMs in `roms/`, and the five that **actually work** do not — their zips are still
inside their own repos (`PELLETINO/pacman.zip`, `QUALIFIER/polepos.zip`,
`TRENCHRUNNER/starwars.zip`, and so on, plus `PELLETINO/mspacman/` which is an
extracted directory, not a zip).

So a build today produces a menu of nine games that cannot run and omits all five
that can. Copy the real ROM sets into `roms/` under their MAME names, then confirm
`./minimame` lists the five working games as included.

Note `mspacman` is a directory, not a zip. Either zip it or teach `configure.py`
to accept a directory — if you do the latter, say so in `README.md`.

**Done means:** `./minimame` shows the five working games in the build, and you
can paste its output.

### 1. Wire `medalboot` into the five working games — P0

`components/medalboot` is the shared contract. Copy it into each game repo (or
point `EXTRA_COMPONENT_DIRS` at MINIMAME's copy — your call, but be consistent and
say which you did). Each game needs exactly three calls, documented in
`ARCHITECTURE.md` under "What a game must do".

| Repo | Game | Slot in `partitions.csv` |
|---|---|---|
| `~/Projects/PELLETINO` | Ms. Pac-Man | `mspacman`, 1 MB |
| `~/Projects/QUALIFIER` | Pole Position | `polepos`, 1 MB |
| `~/Projects/TRENCHRUNNER` | Star Wars | `starwars`, 1 MB |
| `~/Projects/SWARMFIGHTER` | Galaga | `galaga`, 512 KB |
| `~/Projects/CHILOPODA` | Centipede | `centiped`, 512 KB |

`~/Projects/WALKERRUN` (Empire Strikes Back) is slow and not working — wire it up
the same way but do not spend time fixing performance.

`medalboot_game_startup()` must be the genuine first statement of `app_main`,
before any init that can fail. That is what makes a crash land in the menu instead
of boot-looping. Putting it after display or IMU setup silently breaks the safety
property and nothing will complain.

**Done means:** each of the six repos builds clean in Docker, and you can quote the
`binary size` line for each.

### 2. Strip the video and easter eggs — P0

Jesse has cut these. They are the reason two images do not fit.

- **PELLETINO**: 3,815 KB with video, 901 KB without. Its `movie/` directory is
  24 MB of source material. Get the build to the lean path and confirm it fits the
  1 MB `mspacman` slot.
- **TRENCHRUNNER**: `media/media.bin` is 4.87 MB of MJPEG and MP3 for the easter
  egg. Remove the media partition dependency entirely.

**Done means:** both build clean with no media partition referenced, and both
binaries fit their slots with the percentage stated.

### 3. Resolve the partition-table conflict — P0, and it will bite you

Every game repo currently has its own `partitions.csv`. PELLETINO's and
QUALIFIER's still say `Total Flash: 4MB` and carve a single 3.9 MB app — they
were copy-pasted and never updated. The device will hold exactly one partition
table: MINIMAME's.

The trap: IDF's `check_sizes.py` validates an app against the **smallest** app
partition in whatever table that project builds with. If a game builds against
MINIMAME's table, PELLETINO at 901 KB gets checked against the 512 KB `galaga`
slot and **the build fails**, even though its real slot is 1 MB.

IDF app images are relocatable across app partitions, so the likely answer is:
each game keeps a simple single-app `partitions.csv` for building, and only
MINIMAME's table is ever flashed to the device. Verify that claim before relying
on it — check whether any game hardcodes a partition offset or looks up a data
partition that only exists in its own table.

**Done means:** a written answer to "which table does each repo build against, and
why does that not break flashing", plus builds that pass. If the answer is that
game repos need their own single-app table, add a `./minimame` command that emits
one so nobody hand-maintains it.

### 4. Verify and finish `tools/flash_all.sh` — P1

The script exists and `--dry-run` resolves correct offsets and real game binary
paths. Two things are unproven:

- It assumes a game repo builds to `<PROJECT>/build_docker/<lowercase>.bin`.
  That holds for the repos checked so far; confirm it for all of them, and make it
  fail with a clear message rather than silently skipping if the name differs.
- No real flash has ever been run. Do not claim otherwise.

**Done means:** `--dry-run` output for a build containing the five working games,
with every offset and path correct.

### 5. Host preview of the menu — P2, only if time remains

A small Python or SDL harness that renders `menu_render()`'s layout at 240×280 to
a PNG using the real constants and the real blob. Jesse has not seen this screen
and cannot until hardware is in hand. Getting the layout wrong is cheap to fix now
and annoying later.

Do not reimplement the layout by eye — read the constants out of `main/menu.cpp`
so the preview cannot drift from the firmware.

---

## Landmines

- **Byte order.** Band buffers and the marquee blob are RGB565 **big-endian**,
  matching `display_write_preswapped()`. Build colours with `gfx_rgb()`, which
  swaps for you. Writing a raw RGB565 constant into a band gives you wrong colours
  that look plausible enough to miss.
- **DMA cannot read flash.** SPI DMA only reaches internal RAM. That is why
  marquees go through `esp_partition_read()` into a RAM scratch rather than being
  memory-mapped and pushed directly. Do not "optimise" that into an mmap.
- **All 16 OTA slots are used.** `ota_0`–`ota_15` is IDF's hard cap. There is no
  room for a seventeenth game. If one is needed, collapse a pair that shares
  hardware onto one image and select the ROM at boot — PELLETINO already carries
  both Pac-Man and Ms. Pac-Man, WALKERRUN carries Star Wars and Empire Strikes
  Back. Note `pacman` currently has its own 768 KB slot separate from `mspacman`;
  that is a merge candidate, not a second build.
- **`BAT_EN` (GPIO15) must stay HIGH** or the medal cuts its own power.
- **The pre-handshake crash window.** If a game panics before
  `medalboot_game_startup()` runs, control never reaches the launcher, so neither
  the attempts counter nor the power-on escape can rescue it. Keep that call first.

---

## Do not do

- **Do not merge the games into one monolithic image.** This was considered and
  rejected: six mature codebases with diverged `display.cpp`, a large refactor with
  real regression risk on games that currently work, to reclaim ~3 MB of 16 MB that
  is not needed. `ARCHITECTURE.md` records the reasoning.
- **Do not touch `lcd/marquees.bin` by hand.** Regenerate with
  `tools/pack_marquees.py` and confirm it still round-trips.
- **Do not retune the detent or layout constants.** They are unverified, but they
  are unverified *against hardware you do not have*. Changing them blind replaces
  one guess with another and destroys the baseline Jesse will calibrate from.
- **Do not hand-edit `partitions.csv`.** It is generated by `tools/configure.py`
  and overwritten on every build. Change `games.toml` instead.
- **Do not add a `#define`-style build switch.** Galagino's model (edit `config.h`,
  run the converter by hand) was deliberately rejected: presence of a ROM in
  `roms/` is the switch, and the partition table follows from it.
- **Do not delete `lcd/marquees/*.c`** without asking. It is 1.3 MB of generated C
  arrays holding the same pixels as the blob, currently unused — but that is
  Jesse's call, and it is already flagged to him.

---

## Reporting

For each task, report what you **ran** and what it **printed**. A build that was
not run is not a passing build, and "should work" is not a result.

State plainly which tasks you finished, which you started, and which you did not
touch. If something is blocked, say what blocked it and what you would need. Do not
round partial work up to done — the whole point of this handoff is that the next
person can trust the status line without re-deriving it.

Anything you could not verify because there is no hardware, say so explicitly and
list it as such. That list is expected to be non-empty.
