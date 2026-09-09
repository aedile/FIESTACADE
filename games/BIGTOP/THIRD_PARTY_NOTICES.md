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

The machine model in `core/mrdo.c` (memory map, ports, the protection PAL's
answer) and the video in `core/mrdo_video.c` (palette ladder, character and
sprite layout, the sprite colour lookup) are written from MAME's
`src/mame/universal/mrdo.cpp` and `mrdo_v.cpp`. Copyright Nicola Salmoria
and the MAME team; used under the BSD-3-Clause license. `core/sn76489.h` is
this project's own SN76489 model.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
