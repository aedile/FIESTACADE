# Third-party code

## Marat Fayzullin's Z80 emulator (non-commercial)

`core/z80/` is the portable Z80 emulator by Marat Fayzullin, copyright (C)
Marat Fayzullin 1994-2007, from http://fms.komkon.org/EMUL8/. Its terms, from
the source headers: "You are not allowed to distribute this software
commercially. Please, notify me, if you make any changes to this file." The
files are unmodified; the project builds them with `LSB_FIRST` defined.

## MAME (BSD-3-Clause)

The machine model in `core/rallyx.c` (memory map, the LS259 main latch, the
game-supplied interrupt vector) and the video in `core/rallyx_video.c` (the two
tilemaps and how they share one block of RAM, the colour PROM weights and
lookup table, sprite and radar-dot layout, and the shadow palette) are written
from MAME's `src/mame/namco/rallyx.cpp` and `rallyx_v.cpp`. The WSG in
`core/rallyx_sound.c` is written from `src/devices/sound/namco.cpp`. Copyright
Nicola Salmoria, Aaron Giles and the MAME team; used under the BSD-3-Clause
license:

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

## The explosion

The BANG channel is a discrete circuit on the real board, not the WSG. The
model in `core/rallyx_sound.c` is an original reconstruction driven from the
same latch bit, not a port.

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
