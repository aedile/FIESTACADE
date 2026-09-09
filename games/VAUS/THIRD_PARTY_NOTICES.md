# Third-party code

## Marat Fayzullin's Z80 emulator (non-commercial)

`core/z80/` is the portable Z80 emulator by Marat Fayzullin, copyright (C)
Marat Fayzullin 1994-2007, from http://fms.komkon.org/EMUL8/. Its terms, from
the source headers: "You are not allowed to distribute this software
commercially. Please, notify me, if you make any changes to this file." The
files are unmodified; the project builds them with `LSB_FIRST` defined.

## MAME (BSD-3-Clause)

The machine model in `core/arkanoid.c` (memory map, the control latch at
0xD008, the two-semaphore MCU handshake and the DIP wiring on the sound chip's
port B) and the video in `core/arkanoid_video.c` (the tile and sprite layouts,
the colour codes and the two palette banks) are written from MAME's
`src/mame/taito/arkanoid.cpp` and `arkanoid_v.cpp`. Copyright Brad Oliver,
Stephane Humbert and the MAME team; used under the BSD-3-Clause license:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

## The MC6805 core

`core/m6805.h` was written for this project and is covered by the project
license. It is an interpreter for the 68705P5 microcontroller that sits between
the Z80 and its own inputs, and which the game will not run without.

## The AY-3-8910 / YM2149

`core/ay8910.c` was written for this project and is covered by the project
license. The sixteen-step logarithmic volume table is MAME's measured one; the
tone, noise and envelope dividers are taken from the General Instrument
datasheet.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
