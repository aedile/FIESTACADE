# OSTRICH

Joust (Williams, 1982) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal: the
6809 CPU board with its Special Chip 1 blitter and 4-bit bitmap, and the
Williams sound board's 6808 making every sound in software for an 8-bit DAC.
The screen is 292x240; the medal shows the middle 280 columns squeezed to its
240, with a bar above and below. One of the FIESTACADE games; see the
top-level README for how a build is put together.

Controls: twist left and right to run, and BOOT flaps. PWR is a coin (and then
start); hold BOOT 3 s for sound off and on, 10 s for the menu.

Power-up takes the real board's eight seconds of RAM and ROM tests, with the
screen full of the test pattern, before "ALL SYSTEMS GO". The settings memory
is empty at every start, so the program restores its factory settings and asks
for the ADVANCE switch inside the coin door; the medal presses it.

    python3 tools/convert_roms.py joust        # MAME 'joust' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
