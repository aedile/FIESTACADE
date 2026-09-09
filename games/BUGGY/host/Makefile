CC ?= cc
CFLAGS ?= -O2 -Wall -Wno-unused-function -Wno-unused-variable -DLSB_FIRST -I../core -I../core/z80 -I../main/roms -include ../core/z80_fastop.h
SRCS = harness.c ../core/mpatrol.c ../core/mpatrol_video.c ../core/mpatrol_sound.c ../core/ay8910.c ../core/z80/Z80.c
harness: $(SRCS) ../core/*.h ../main/roms/mpatrolw_roms.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) -lm
clean:
	rm -f harness
