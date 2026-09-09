# PLUMBER

Mario Bros. (Nintendo, 1983) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal.
Donkey Kong's kind of board grown up: a Z80, an 8039 that plays the tunes into
a DAC, a scrolling character map and three-plane sprites. The monitor is the
normal way up, so the picture is letterboxed on the portrait panel. One of the
FIESTACADE games; see the top-level README for how a build is put together.

Controls: twist left and right to run, BOOT jumps. PWR is a coin (and then
start); hold BOOT 3 s for sound off and on, 10 s for the menu.

    python3 tools/convert_roms.py mario        # MAME 'mario' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
