# Third-party code

## Andre Weissflog's 6502 (zlib/libpng)

`core/m6502.h` is the cycle-stepped 6502 from the chips project, copyright (c)
2018 Andre Weissflog, under the zlib/libpng license. Centipede's video hardware
steals bus cycles from the CPU, so this game keeps the cycle-stepped core rather
than the instruction-stepped one used elsewhere in this family of projects.

## MAME (BSD-3-Clause)

The machine model in `core/centiped.c` — memory map, the four-per-frame IRQ from
/16V, the trackball counters and the EAROM stub — and the video in
`core/centiped_video.c` — palette decoding, tilemap and sprite layout, and the
playfield/sprite priority — are written from MAME's
`src/mame/atari/centiped.cpp`. `core/pokey.c` is written from
`src/devices/sound/pokey.cpp`. Copyright Nicola Salmoria, Aaron Giles and the
MAME team; used under the BSD-3-Clause license:

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

## ROMs

No game ROMs are included in this repository, and none ever will be. Supplying
them is up to whoever builds the thing.
