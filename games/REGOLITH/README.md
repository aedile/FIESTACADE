# REGOLITH

Lunar Lander (Atari, 1979) on the Waveshare ESP32-C6-LCD-1.69 Fiesta medal.
Asteroids' board a few months earlier - the same 6502 and Digital Vector
Generator - with an analogue throttle. One of the FIESTACADE games; see the
top-level README for how a build is put together.

Controls: twist to rotate the lander, tip the medal away from you to open the
throttle (all the way over is full burn), BOOT is ABORT. PWR is a coin (and
then start, so the mission is the cabinet's default); hold BOOT 3 s for sound
off and on, 10 s for the menu.

    python3 tools/convert_roms.py llander      # MAME 'llander' set, extracted
    make -C host && host/harness out 20        # on the host, frames to out/
    docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.3.4 \
        idf.py -B build_docker -DIDF_TARGET=esp32c6 build
