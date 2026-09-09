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

The machine model in `core/mario.c` (memory map, latches), the video in
`core/mario_video.c` (colour PROM weights, character and sprite layout and
colour selection) and the sound board wiring in `core/mario_sound.c` (how the
8039 reaches its latch, its sample pages and its trigger lines) are written from
MAME's `src/mame/nintendo/mario.cpp`, `mario_v.cpp` and `mario_a.cpp`.
Copyright Mirko Buffoni and the MAME team; used under the BSD-3-Clause license.
The discrete footstep and skid circuits are synthesised here.

## The 8039 core

`core/mcs48.h` is this project's own instruction-stepped MCS-48 core (from
GIRDER, with a port 1 input hook added).

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
