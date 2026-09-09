# BIGTOP

Mr. Do! (Universal, 1982) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal. A
Z80, two character maps, sixteen-colour sprites and two SN76489s. One of the
FIESTACADE games; see the top-level README for how a build is put together.

Controls: twist left and right, tip away and toward you, to dig in one of the
four directions; BOOT throws the power ball. PWR is a coin (and then start);
hold BOOT 3 s for sound off and on, 10 s for the menu.

    python3 tools/convert_roms.py mrdo         # MAME 'mrdo' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
