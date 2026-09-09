# CHILOPODA

**Atari's 1980 Centipede, emulated on an ESP32-C6 Fiesta medal.** A 6502, POKEY
sound, and a trackball driven by tilt — held upright, running at full speed.

*Chilopoda* is the scientific class that centipedes belong to.

A San Antonio Fiesta medal is a collectible pin. This one plays Centipede.

---

## 🎮 Quick Start Guide

### How to Play

You hold the medal upright, like a phone. Centipede's monitor was vertical, so
the picture stands up the same way you do.

| Control | What it does |
|---|---|
| **Tilt left / right** | Move sideways |
| **Tilt toward / away from you** | Move up and down the field |
| **Middle button** | Fire |
| **Power button, short press** | Insert a coin and start |
| **Power button, hold 1 second** | Power off |

Centipede is a **trackball** game, so tilt drives the trackball counters rather
than a joystick: the *angle* sets the speed, which is the closest a tilt sensor
gets to a ball you spin. Tilt further to move faster.

Tilt is measured against however you are holding it right now. Coin up to
re-centre.

### Charging

USB-C. Holding the power button for a second cuts the battery rail.

### Troubleshooting

**Up and down don't work.** They should — this was the first bug found on
hardware and it is fixed. If it recurs, the neutral pose is stale: coin up.

**Movement is jerky at low speed.** Fractional trackball counts are accumulated
in fixed point so slow tilts still move, one count at a time. Very slow tilts
will look stepped, because the hardware counter is 4 bits.

---

## 🔨 Building Your Own

```sh
git clone https://github.com/aedile/CHILOPODA.git
cd CHILOPODA
python3 tools/convert_roms.py /path/to/centiped
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
    idf.py -B build_docker build
cd build_docker && esptool --chip esp32c6 -p /dev/cu.usbmodemXXXX \
    -b 460800 write_flash @flash_args
```

Flashing has to run **from inside `build_docker`** and **from the host** —
Docker Desktop on macOS cannot reach USB.

### The ROMs

Not included. You need MAME's `centiped` set. The converter checks every CRC.

---

## 🔬 Technical Details

### The original hardware

One 6502 at 1.512 MHz, one POKEY, a trackball, one fire button. A 32×30 tilemap
of 8×8 characters with 16 sprites over it, and a 4-bit-per-pixel palette out of
a small PROM. Four interrupts a frame, clocked from /16V.

### Tilt as a trackball

A trackball reports *motion*, not position, and there is no natural mapping from
a tilt sensor to one. What works is treating the tilt angle as a velocity: past
a small deadband, the further you tilt the more counts per frame the game sees.

Two things about that were harder than expected.

**Both axes have to be measured against a captured neutral pose.** Held upright,
gravity lies *in the plane of the panel*, so the raw pitch the sensor reports is
pinned near its limit and cannot swing both ways. That is exactly why up and
down did nothing on the first hardware test. Measuring against "however I am
holding it right now" — `atan2(ay, ax)` for the in-plane direction and
`atan2(az, hypot(ax, ay))` for how far gravity leaves that plane — makes the
second axis usable at all.

**A frame's motion has to be handed over in quarters.** The game reads a 4-bit
counter four times a frame. Applying a whole frame's worth of movement at once
aliases badly against that, and the first attempt at splitting it was stateful
and never reset its remainder, which broke movement entirely. The working
version is stateless: each of the four interrupts delivers
`want × (q+1)/4 − want × q/4`.

### The idle skip

The main loop waits for its frame tick at `2015` with `LSR $8A / BCC`. Skipping
those cycles is worth a third of the frame — but it is only sound while `$8A` is
**zero**, because `LSR` rewrites the byte and any other value would fall out of
the loop on a later pass. The first version of this check tested only the low
bit and was quietly wrong.

### Why this one keeps the slow CPU core

Elsewhere in this family of projects the cycle-stepped 6502 was replaced with an
instruction-stepped one for speed. Centipede keeps the cycle-stepped core: its
video hardware steals bus cycles from the CPU, and that is not something an
instruction-stepped model represents. It fits in the budget here, so it stays.

---

## 📁 Project Structure

```
core/           platform-independent emulation, shared with the host harness
  centiped.c      memory map, IRQ timing, trackball, idle skip
  centiped_video.c tilemap, sprites, palette
  pokey.c         POKEY sound
  m6502.h         cycle-stepped 6502 (chips project, zlib)
main/           the ESP32 application
components/     display, IMU and audio HAL for the Waveshare board
host/           builds the same core on a desktop; frames to PPM, audio to WAV
tools/          ROM converter
```

## ⚙️ Configuration

| What | Where |
|---|---|
| DIP switches | `ce_set_dips()` in `main/main.cpp` |
| Tilt sensitivity | `DEADBAND_DEG`, `FULL_DEG`, `MAX_COUNTS` in `main/input.cpp` |
| Tilt direction | `X_SIGN`, `Y_SIGN` in `main/input.cpp` |

## 📌 Status and Known Gaps

Runs at full speed with sound.

- The EAROM (high-score storage) is a stub; scores do not persist.
- Cocktail flip-screen is not implemented.

## 📄 Legal Notice

### ROM files

No ROMs here. Centipede is © 1980 Atari. This project ships a converter, not a
game.

### Third-party code

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The machine model and
video are written from MAME (BSD-3-Clause); the 6502 is from the chips project
(zlib).

### Disclaimer

Not affiliated with, endorsed by, or connected to Atari, its successors, or the
Fiesta San Antonio Commission.

## 📜 License

[0BSD](LICENSE) — no attribution required, no conditions.
