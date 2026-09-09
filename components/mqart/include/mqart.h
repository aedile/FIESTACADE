/*
 * mqart.h - marquee artwork stored in the "mqart" flash partition.
 *
 * The blob is built by tools/pack_marquees.py: a fixed-size header and entry
 * table followed by raw pixels. Pixels are RGB565 stored BIG-ENDIAN, i.e. already
 * in the byte order the ST7789 wants, so they go straight to
 * display_write_preswapped() with no per-pixel swap.
 *
 * Reads go through esp_partition_read() rather than a memory map: it lands the
 * pixels in RAM (which SPI DMA can reach, unlike flash-mapped addresses) and
 * avoids the MMU's limits on how much can be mapped at once.
 */
#pragma once
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MQART_BOX_W     208     /* every marquee is fitted inside this box */
#define MQART_BOX_H     104
#define MQART_MAX       24      /* entries we are willing to hold in RAM */

typedef struct {
    char     rom[13];           /* MAME ROM name; what the launcher records as the selection */
    char     boot[13];          /* partition label to chain-boot; == rom except for a shared slot */
    char     title[25];         /* display name, e.g. "Ms. Pac-Man" */
    uint16_t w, h;              /* fitted size; w <= 208, h <= 104 */
    uint32_t off, len;          /* byte range within the partition */
} mqart_entry_t;

esp_err_t             mqart_init(void);
int                   mqart_count(void);
const mqart_entry_t  *mqart_get(int i);
int                   mqart_find(const char *rom);   /* index, or -1 */

/* The partition label to chain-boot for a given selection ROM. Normally the ROM
 * itself; for a shared slot (Pac-Man riding Ms. Pac-Man's image) it is the slot
 * owner's label. Returns rom unchanged if it is not in the blob. */
const char           *mqart_boot_label(const char *rom);

/* Copy nrows of pixels beginning at row into dst (nrows * e->w * 2 bytes). */
esp_err_t mqart_read_rows(const mqart_entry_t *e, int row, int nrows, void *dst);

#ifdef __cplusplus
}
#endif
