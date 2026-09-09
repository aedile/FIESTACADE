# Third-party code

## vecx 6809 core (GPL-3.0)

`core/e6809.c` and `core/e6809.h` are the MC6809 emulator from vecx, the
Vectrex emulator by Valavan Manohararajah, as maintained at
https://github.com/jhawthorn/vecx. vecx is distributed under the GNU General
Public License version 3; the full text is in `LICENSES/GPL-3.0.txt`. The
copy here is TRENCHRUNNER's, with its modifications: bus accessors and the
interrupt line can be supplied as macros by the including file so they inline,
and there is a program counter accessor.

## MAME (BSD-3-Clause)

The machine model in `core/joust.c` (memory map, the ROM bank over the video
RAM, the PIAs and their interrupt wiring, the scanline interrupts, the blitter
and its timing), the video in `core/joust_video.c` (bitmap layout, palette
resistor weights) and the sound board in `core/joust_sound.c` (its map and
clock) are written from MAME's `src/mame/williams/williams.cpp`,
`williams_m.cpp`, `williams_v.cpp` and `williamsblitter.cpp`. Copyright Aaron
Giles, Michael Soderstrom, Marc LaFontaine, Sean Riddle and the MAME team; used
under the BSD-3-Clause license. `core/m6800.h` is this project's own 6800.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
