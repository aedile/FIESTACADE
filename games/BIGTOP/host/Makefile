CC ?= cc
CFLAGS ?= -O2 -Wall -Wno-unused-function -Wno-unused-variable -DLSB_FIRST -I../core -I../core/z80 -I../main/roms
SRCS = harness.c ../core/mrdo.c ../core/mrdo_video.c ../core/z80/Z80.c
harness: $(SRCS) ../core/*.h ../main/roms/mrdo_roms.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lm
clean:
	rm -f harness
