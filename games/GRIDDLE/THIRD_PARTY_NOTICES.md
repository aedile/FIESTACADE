# Third-party code

## MAME (BSD-3-Clause)

The machine model in `core/btime.c` and `core/btime_sound.c` (memory maps, the
CPU-7 opcode scrambling rule, the sound board's latch, interrupt and NMI
pacing) and the video in `core/btime_video.c` (character, sprite and tile
layouts, the background map, the palette format) are written from MAME's
`src/mame/dataeast/btime.cpp` and its DECO CPU-7 device. Copyright Zsolt
Vasvari, Couriersud, Nicola Salmoria and the MAME team; used under the
BSD-3-Clause license. `core/m6502fast.h` is this project's own 6502 core (from
AEROLITE, with an opcode-fetch hook added) and `core/ay8910.c` its own AY-3-8910.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
