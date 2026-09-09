# KEG

Root Beer Tapper (Bally Midway, 1984) on the Waveshare ESP32-C6-LCD-1.69 Fiesta
medal: the MCR 91490 board, the 91464 sprite generator and the SSIO sound board
with its two AY-3-8910s. The hardware draws 512x480; KEG draws it at half that,
which is what the panel can show, with the sprites read from their 128 KB of
ROM as they are drawn. One of the FIESTACADE games; see the top-level README
for how a build is put together.

Controls: twist left and right to move along the bar, tip away and toward you
to change bars, and BOOT pours - hold it to fill a glass. PWR is a coin (and
then start); hold BOOT 3 s for sound off and on, 10 s for the menu.

    python3 tools/convert_roms.py rbtapper     # MAME 'rbtapper' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
