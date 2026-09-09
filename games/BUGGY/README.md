# BUGGY

Moon Patrol (Irem, 1982; the Williams-licensed US set) on the Waveshare
ESP32-C6-LCD-1.69 Fiesta medal: the M52 board's Z80, its three scrolling
paintings, its character map and sprites, and the M52 sound board's 6803 with
two AY-3-8910s and an MSM5205. The picture is 240x252, exactly the panel's
width, with a bar above and below. One of the FIESTACADE games; see the
top-level README for how a build is put together.

Controls: twist left and right to slow down and speed up, and BOOT jumps. The
guns fire by themselves. PWR is a coin (and then start); hold BOOT 3 s for
sound off and on, 10 s for the menu.

    python3 tools/convert_roms.py mpatrolw     # MAME 'mpatrolw' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
