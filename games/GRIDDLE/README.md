# GRIDDLE

Burger Time (Data East, 1982) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal.
Two 6502s - one in DECO's CPU-7 block that scrambles its opcodes, one on the
sound board with two AY-3-8910s - a three-plane character map, eight sprites and
a sixteen-register palette. One of the FIESTACADE games; see the top-level
README for how a build is put together.

Controls: twist left and right, tip away and toward you, to walk; BOOT shakes
the pepper. PWR is a coin (and then start); hold BOOT 3 s for sound off and on,
10 s for the menu.

    python3 tools/convert_roms.py btime        # MAME 'btime' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
