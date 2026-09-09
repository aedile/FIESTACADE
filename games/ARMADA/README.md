# ARMADA

Galaxian (Namco/Midway, 1979) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal.
A Z80, a per-column-scrolling tilemap, eight sprites, eight shells and the
shift-register starfield, on the board Frogger was later built from. One of
the FIESTACADE games; see the top-level README for how a build is put together.

Controls: twist left and right to move the fighter, BOOT fires. PWR is a coin
(and then start); hold BOOT 3 s for sound off and on, 10 s for the menu.

The sound board was discrete; ARMADA synthesises each circuit to the same
shape - the three-octave hum, the dive whistle, the shot, the explosion - but
they are approximations.

    python3 tools/convert_roms.py galaxian     # MAME 'galaxian' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
