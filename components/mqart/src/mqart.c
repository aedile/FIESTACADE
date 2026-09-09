#include "mqart.h"
#include "esp_partition.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqart";

#define MQART_MAGIC     "MQ03"
#define MQART_SUBTYPE   0x40
#define DISK_ENTRY_SZ   60      /* 12s rom, 12s boot, 24s title, u16 w, u16 h, u32 off, u32 len */

static const esp_partition_t *s_part;
static mqart_entry_t s_entries[MQART_MAX];
static int s_count;

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

esp_err_t mqart_init(void)
{
    s_count = 0;
    s_part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                                      (esp_partition_subtype_t)MQART_SUBTYPE, "mqart");
    if (!s_part) {
        ESP_LOGE(TAG, "no 'mqart' partition - flash it with tools/flash_mqart.sh");
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t hdr[8];
    esp_err_t err = esp_partition_read(s_part, 0, hdr, sizeof hdr);
    if (err != ESP_OK) return err;
    if (memcmp(hdr, MQART_MAGIC, 4) != 0) {
        ESP_LOGE(TAG, "bad magic %.4s (want %s) - partition not flashed?", (char *)hdr, MQART_MAGIC);
        return ESP_ERR_INVALID_STATE;
    }

    int count = rd16(hdr + 4);
    int esz   = rd16(hdr + 6);
    if (esz != DISK_ENTRY_SZ) {
        ESP_LOGE(TAG, "entry size %d, expected %d", esz, DISK_ENTRY_SZ);
        return ESP_ERR_INVALID_VERSION;
    }
    if (count > MQART_MAX) {
        ESP_LOGW(TAG, "blob holds %d marquees, keeping first %d", count, MQART_MAX);
        count = MQART_MAX;
    }

    for (int i = 0; i < count; i++) {
        uint8_t e[DISK_ENTRY_SZ];
        err = esp_partition_read(s_part, 8 + (size_t)i * DISK_ENTRY_SZ, e, sizeof e);
        if (err != ESP_OK) return err;

        mqart_entry_t *m = &s_entries[s_count];
        memcpy(m->rom, e, 12);        m->rom[12]   = 0;
        memcpy(m->boot, e + 12, 12);  m->boot[12]  = 0;
        memcpy(m->title, e + 24, 24); m->title[24] = 0;
        m->w   = rd16(e + 48);
        m->h   = rd16(e + 50);
        m->off = rd32(e + 52);
        m->len = rd32(e + 56);
        if (m->boot[0] == 0) memcpy(m->boot, m->rom, sizeof m->boot);   /* default: boot self */

        /* Refuse anything that would read past the partition or overflow the box. */
        if (m->w == 0 || m->h == 0 || m->w > MQART_BOX_W || m->h > MQART_BOX_H ||
            m->len != (uint32_t)m->w * m->h * 2 || m->off + m->len > s_part->size) {
            ESP_LOGW(TAG, "entry %d (%s) is malformed, skipping", i, m->rom);
            continue;
        }
        s_count++;
    }

    ESP_LOGI(TAG, "%d marquees in %u KB partition", s_count, (unsigned)(s_part->size / 1024));
    return s_count ? ESP_OK : ESP_ERR_INVALID_STATE;
}

int mqart_count(void) { return s_count; }

const mqart_entry_t *mqart_get(int i)
{
    return (i >= 0 && i < s_count) ? &s_entries[i] : NULL;
}

int mqart_find(const char *rom)
{
    for (int i = 0; i < s_count; i++)
        if (strcmp(s_entries[i].rom, rom) == 0) return i;
    return -1;
}

const char *mqart_boot_label(const char *rom)
{
    int i = mqart_find(rom);
    return (i >= 0 && s_entries[i].boot[0]) ? s_entries[i].boot : rom;
}

esp_err_t mqart_read_rows(const mqart_entry_t *e, int row, int nrows, void *dst)
{
    if (!s_part || !e || row < 0 || nrows <= 0 || row + nrows > e->h) return ESP_ERR_INVALID_ARG;
    size_t stride = (size_t)e->w * 2;
    return esp_partition_read(s_part, e->off + (size_t)row * stride, dst, stride * nrows);
}
