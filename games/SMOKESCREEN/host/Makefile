CC ?= cc
CFLAGS ?= -O2 -Wall -Wno-unused-function -Wno-unused-variable -DLSB_FIRST -I../core -I../core/z80 -I../main/roms
SRCS = harness.c ../core/rallyx.c ../core/rallyx_video.c ../core/rallyx_sound.c ../core/z80/Z80.c
harness: $(SRCS) ../core/*.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lm
clean:
	rm -f harness
