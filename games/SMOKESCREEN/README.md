# SMOKESCREEN

**Namco's 1980 Rally-X, emulated on an ESP32-C6 Fiesta medal.** A Z80, the
Namco WSG, two tilemaps sharing one block of RAM, and the discrete explosion
circuit — running at the cabinet's 60.6 Hz with every frame drawn.

A San Antonio Fiesta medal is a collectible pin. This one plays Rally-X.

---

## 🎮 Quick Start Guide

### How to Play

You hold the medal upright, like a phone. Drive the blue car, collect all ten
flags, dodge the red cars and the rocks, and don't run out of fuel.

| Control | What it does |
|---|---|
| **Tilt left / right** | Steer left / right |
| **Tilt away / toward you** | Steer up / down |
| **Middle button** | Lay a smoke screen |
| **Power button, short press** | Insert a coin and start |
| **Power button, hold 1 second** | Power off |

Rally-X had a four-way gate on the joystick, so only the axis you have tilted
further counts — a diagonal never produces two directions. The smoke screen
costs fuel and makes the car chasing you spin out.

The radar strip down the right-hand quarter shows the whole maze: yellow dots
are flags, red dots are the cars hunting you.

Tilt is measured against however you are holding it right now. Coin up to
re-centre.

### Charging

USB-C. Holding the power button for a second cuts the battery rail.

### Troubleshooting

**The car steers the wrong way.** Each axis is one sign in `main/input.cpp`
(`X_SIGN`, `Y_SIGN`).

**It won't steer diagonally.** Correct — the cabinet's stick was gated to four
directions and this reproduces that.

---

## 🔨 Building Your Own

```sh
git clone https://github.com/aedile/SMOKESCREEN.git
cd SMOKESCREEN
python3 tools/convert_roms.py /path/to/rallyx
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
    idf.py -B build_docker build
cd build_docker && esptool --chip esp32c6 -p /dev/cu.usbmodemXXXX \
    -b 460800 write_flash @flash_args
```

Flashing has to run **from inside `build_docker`** and **from the host** —
Docker Desktop on macOS cannot reach USB.

### The ROMs

Not included. You need MAME's `rallyx` set: `1b`, `rallyxn.1e`, `rallyxn.1h`
and `rallyxn.1k` (program), `8e` (graphics), `rx1-6.8m` (dots), `rx1-1.11n`
(palette), `rx1-7.8p` (colour lookup) and `rx1-5.3p` (WSG waveforms). The
converter checks every CRC.

---

## 🔬 Technical Details

### The original hardware

One Z80 at 3.072 MHz, the Namco 3-voice WSG — the same part Pac-Man and Galaga
use, written through the same register layout — and a discrete circuit for the
explosion, gated by one bit of an LS259 latch and mixed in after the WSG.

The screen is 288×224 across a horizontal monitor.

### One block of RAM doing four jobs

All the video comes out of the single 4 KB block at 0x8000, which is unusually
crowded:

```
0x000-0x3FF   radar strip tile codes — and, in the first 64 bytes,
              the sprites (0x14-0x1F) and the radar dots (0x20-0x3F)
0x400-0x7FF   playfield tile codes
0x800-0xBFF   radar strip attributes, and the dot Y positions
0xC00-0xFFF   playfield attributes
```

Sprites and dots overlap the top of the radar's tile codes. The game simply
never draws radar tiles there.

The playfield is a 32×32 tilemap that scrolls in both directions, clipped to
the left 28 columns. The radar is an 8×32 strip that the hardware repeats
across the screen and clips to the right quarter — so at screen x 224 you are
looking at the *second half* of that strip, and at x 256 the first half again.
It looks like a mistake and it is exactly what the hardware does.

Colour goes through two PROMs. A pixel and its tile's colour index a 256-byte
lookup that yields one of 16 entries, and those index a 32-entry palette. The
upper 16 palette entries are shadowed copies of the lower 16, which is how the
smoke screen darkens whatever is underneath it.

### The bug that cost the most time

The game booted, passed its ROM test, ran its RAM test — and then wandered off
and sat on an RST vector at 0x0030 forever.

The vblank interrupt is a **level on a wire**: it stays asserted until the CPU
acknowledges it. Raising it once at the end of a frame and calling the CPU
immediately throws it away whenever the game happens to have interrupts
disabled at that instant. And this game does disable them — at which point it
loses a frame tick, runs off into memory it never meant to execute, and lands
somewhere like an RST vector.

Holding it pending and delivering it at the first slice boundary where the CPU
will take it fixed the whole thing in one change. It is the same mistake, in a
different place, as Frogger's sound interrupt.

Rally-X also supplies **its own interrupt vector** by writing it to port 0,
which is how it steers its IM 2 table.

### Speed

Full speed with no tricks needed — one Z80 at 3 MHz is comfortable. The
character and sprite ROMs are expanded once at init into ready-made pixel
values, which keeps the renderer's inner loop to a single load, the same
technique that mattered a great deal more on Frogger.

---

## 📁 Project Structure

```
core/           platform-independent emulation, shared with the host harness
  rallyx.c        Z80, memory map, the LS259 latch, interrupt timing
  rallyx_video.c  two tilemaps, sprites, radar dots, the shadow palette
  rallyx_sound.c  Namco WSG and the discrete BANG
  z80/            Marat Fayzullin's Z80 (non-commercial — see notices)
main/           the ESP32 application
components/     display, IMU and audio HAL for the Waveshare board
host/           builds the same core on a desktop; frames to PPM, audio to WAV
tools/          ROM converter
```

## 💻 Running It on Your Computer

```sh
cd host && make
./harness /tmp/out 45 --every 3 --wav /tmp/rx.wav \
    --script "30.0:coin=1,30.2:coin=0,31.0:start=1,31.2:start=0,34.0:right=1,38.0:smoke=1"
```

Script keys: `coin`, `start`, `up`, `down`, `left`, `right`, `smoke`. The
attract cycle takes about thirty seconds to reach the instructions screen, so
give it time before coining up.

## ⚙️ Configuration

| What | Where |
|---|---|
| DIP switches | `rx_set_dips()` in `main/main.cpp` — `0xcb` is the factory setting |
| Picture size and shape | `PIC_W`, `PIC_H`, `TOP_BAR` in `main/render.cpp` |
| Tilt direction | `X_SIGN`, `Y_SIGN` in `main/input.cpp` |
| Steering threshold | `STEER_DEG` in `main/input.cpp` |

## 📌 Status and Known Gaps

Runs at full speed with sound, every frame drawn, nothing skipped or dropped.

- The explosion is a reconstruction of a discrete circuit, not a netlist.
- Cocktail flip-screen is read from the latch but not applied.
- The watchdog is ignored rather than modelled.

## 📄 Legal Notice

### ROM files

No ROMs here. Rally-X is © 1980 Namco. This project ships a converter, not a
game.

### Third-party code

See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The machine model, video
and WSG are written from MAME (BSD-3-Clause). **`core/z80/` is Marat
Fayzullin's Z80 emulator, which its author licenses for non-commercial use
only** — that term applies to this repository as a whole in any commercial
context.

### Disclaimer

Not affiliated with, endorsed by, or connected to Namco, Bandai Namco, their
successors, or the Fiesta San Antonio Commission.

## 📜 License

[0BSD](LICENSE) for the project's own code — but see the note above about the
Z80 core, which is non-commercial.
