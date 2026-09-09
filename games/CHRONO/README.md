# CHRONO

Time Pilot (Konami, 1982) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal. Two
Z80s - one for the game, one on the Pooyan sound board with its two
AY-3-8910s - a character map, 24 sprites and the lookup PROMs. One of the
FIESTACADE games; see the top-level README for how a build is put together.

Controls: twist and tip the medal to point the plane - it turns toward the
direction you hold, diagonals included - and BOOT fires. PWR is a coin (and
then start); hold BOOT 3 s for sound off and on, 10 s for the menu.

    python3 tools/convert_roms.py timeplt      # MAME 'timeplt' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
