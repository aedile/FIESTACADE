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

The machine model in `core/invaders.c` (memory map, the I/O ports, the shift
register, the two half-frame interrupts and the position of the colour gel) is
written from MAME's `src/mame/midw8080/mw8080bw.cpp` and `8080bw_v.cpp`.
Copyright Nicola Salmoria, Tormod Tjaberg and the MAME team; used under the
BSD-3-Clause license. The sound board in `core/invaders_sound.c` is an original
synthesis of the analogue circuits; MAME plays samples instead.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
