/* Just enough ESP-IDF to compile the launcher's UI on a desktop. */
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NOT_FOUND        0x105
#define ESP_ERR_INVALID_STATE    0x103
#define ESP_ERR_INVALID_ARG      0x102
#define ESP_ERR_INVALID_VERSION  0x10A
#define ESP_ERR_INVALID_SIZE     0x104

#define MALLOC_CAP_DMA  0
#define MALLOC_CAP_8BIT 0
static inline void *heap_caps_malloc(size_t n, int caps)
{
    (void)caps;
#ifdef PREVIEW_STRIPS
    if (n > 100000) return NULL;   /* pretend the whole-screen buffer would not fit */
#endif
    return malloc(n);
}

#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "E %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "W %s: " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) fprintf(stderr, "I %s: " fmt "\n", tag, ##__VA_ARGS__)

typedef enum { ESP_PARTITION_TYPE_APP = 0, ESP_PARTITION_TYPE_DATA = 1 } esp_partition_type_t;
typedef int esp_partition_subtype_t;
typedef struct { size_t size; } esp_partition_t;

const esp_partition_t *esp_partition_find_first(esp_partition_type_t t, esp_partition_subtype_t s, const char *label);
esp_err_t esp_partition_read(const esp_partition_t *p, size_t off, void *dst, size_t n);
