# Third-party code

## Marat Fayzullin's Z80 emulator (non-commercial)

`core/z80/` is the portable Z80 emulator by Marat Fayzullin, copyright (C)
Marat Fayzullin 1994-2007, from http://fms.komkon.org/EMUL8/. Its terms, from
the source headers: "You are not allowed to distribute this software
commercially. Please, notify me, if you make any changes to this file." The
files are unmodified; the project builds them with `LSB_FIRST` defined. Space
Invaders is an 8080 program; the Z80 is a superset and the program uses nothing
that differs.

## MAME (BSD-3-Clause)

The machine model in `core/tapper.c` (memory and port maps, the CTC's use, the
SSIO board's latches, interrupt clock and mixer ports) and the video in
`core/tapper_video.c` (tile and sprite layouts, the palette, the sprite
priority rules) are written from MAME's `src/mame/bally/mcr.cpp`, `mcr_v.cpp`,
`mcr_m.cpp` and `midway_sound.cpp`. Copyright Aaron Giles, Christopher Kirmse
and the MAME team; used under the BSD-3-Clause license. `core/ay8910.c` is
this project's own AY-3-8910.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
