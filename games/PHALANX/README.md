# PHALANX

Space Invaders (Taito/Midway, 1978) on the Waveshare ESP32-C6-LCD-1.69 Fiesta
medal. An 8080 (run on a Z80 core), an 8 KB program, a 1-bit frame the CPU
draws itself, and the colour gel from the cabinet glass. One of the FIESTACADE
games; see the top-level README for how a build is put together.

Controls: twist left and right to move the cannon, BOOT fires. PWR is a coin
(and then start); hold BOOT 3 s for sound off and on, 10 s for the menu.

The sound board was analogue and MAME plays recordings of it; PHALANX
synthesises each sound to the same shape instead, so the UFO warbles and the
fleet thumps its four notes, but they are approximations.

    python3 tools/convert_roms.py invaders     # MAME 'invaders' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
