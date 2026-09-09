# Third-party code

## Marat Fayzullin's Z80 emulator (non-commercial)

`core/z80/` is the portable Z80 emulator by Marat Fayzullin, copyright (C)
Marat Fayzullin 1994-2007, from http://fms.komkon.org/EMUL8/. Its terms, from
the source headers: "You are not allowed to distribute this software
commercially. Please, notify me, if you make any changes to this file." The
files are unmodified; the project builds them with `LSB_FIRST` defined.

## MAME (BSD-3-Clause)

The machine model in `core/mpatrol.c` (memory and port maps, the protection
read, the interrupt), the video in `core/mpatrol_video.c` (layouts, palette
PROM decoding and resistor weights, layer placement, sprite order) and the
sound board in `core/mpatrol_sound.c` (the 6803's port wiring to the AYs, the
ADPCM chip's control bits and its clock) are written from MAME's
`src/mame/irem/m52.cpp` and `irem.cpp`. Copyright Nicola Salmoria and the MAME
team; used under the BSD-3-Clause license. `core/ay8910.c` and `core/m6800.h`
are this project's own AY-3-8910 and 6800/6803.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
