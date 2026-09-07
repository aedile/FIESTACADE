/*
 * preview.c - render the launcher's menu on a desktop, at the real 240x280, using the real
 * constants out of main/menu.cpp and the real artwork blob. Writes PPMs.
 *
 * The point is that nothing here reimplements the layout: menu.cpp is compiled as-is and the
 * only things stubbed are the flash partition, the panel and the buttons.
 */
#include "stubs.h"
#include "gfx.h"
#include "mqart.h"
#include "menu.h"
#include "display.h"
#include "input.h"
#include "games.h"

/* --- the artwork blob, out of a file rather than a partition --- */
static FILE *blob;
static esp_partition_t blob_part;

const esp_partition_t *esp_partition_find_first(esp_partition_type_t t, esp_partition_subtype_t s, const char *label)
{
    (void)t; (void)s; (void)label;
    return blob ? &blob_part : NULL;
}

esp_err_t esp_partition_read(const esp_partition_t *p, size_t off, void *dst, size_t n)
{
    (void)p;
    if (!blob || fseek(blob, (long)off, SEEK_SET) != 0) return ESP_FAIL;
    return fread(dst, 1, n, blob) == n ? ESP_OK : ESP_FAIL;
}

/* --- the panel: collect the bands into one image --- */
static uint16_t screen[GFX_H][GFX_W];
static int win_y, win_h, win_row;

void display_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    (void)x; (void)w;
    win_y = y; win_h = h; win_row = 0;
}

void display_write_preswapped(const uint16_t *px, uint32_t n)
{
    for (uint32_t i = 0; i < n; i++) {
        int row = win_y + win_row + (int)(i / GFX_W);
        int col = (int)(i % GFX_W);
        if (row >= 0 && row < GFX_H && col >= 0 && col < GFX_W) screen[row][col] = px[i];
    }
    win_row += (int)(n / GFX_W);
}

void display_wait_done(void) { }

/* --- the buttons --- */
static int hold_ms;
int  input_hold_ms(void) { return hold_ms; }

/* --- which games are installed: the preview says all of them --- */
static int all_installed = 1;
bool game_installed(const char *rom) { (void)rom; return all_installed; }

static void write_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", GFX_W, GFX_H);
    for (int y = 0; y < GFX_H; y++) {
        for (int x = 0; x < GFX_W; x++) {
            uint16_t be = screen[y][x];
            uint16_t c = (uint16_t)((be >> 8) | (be << 8));   /* the band is big-endian */
            uint8_t rgb[3] = { (uint8_t)(((c >> 11) & 0x1f) << 3),
                               (uint8_t)(((c >> 5) & 0x3f) << 2),
                               (uint8_t)((c & 0x1f) << 3) };
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}

int main(int argc, char **argv)
{
    const char *blobpath = argc > 1 ? argv[1] : "../lcd/marquees.bin";
    const char *outdir   = argc > 2 ? argv[2] : "/tmp";
    blob = fopen(blobpath, "rb");
    if (!blob) { fprintf(stderr, "cannot open %s\n", blobpath); return 1; }
    fseek(blob, 0, SEEK_END); blob_part.size = (size_t)ftell(blob); fseek(blob, 0, SEEK_SET);

    if (mqart_init() != ESP_OK) { fprintf(stderr, "mqart_init failed\n"); return 1; }
    menu_init();

    char path[512];
    int n = mqart_count();
    for (int i = 0; i < n; i++) {
        menu_select_rom(mqart_get(i)->rom);
        memset(screen, 0, sizeof screen);
        menu_render();
        snprintf(path, sizeof path, "%s/menu_%02d_%s.ppm", outdir, i, mqart_get(i)->rom);
        write_ppm(path);
    }

    /* the states the browse screen has: not installed, mid-hold, launching */
    menu_select_rom(mqart_get(0)->rom);
    all_installed = 0;
    memset(screen, 0, sizeof screen); menu_render();
    snprintf(path, sizeof path, "%s/menu_state_absent.ppm", outdir); write_ppm(path);
    all_installed = 1;

    hold_ms = 900;
    memset(screen, 0, sizeof screen); menu_render();
    snprintf(path, sizeof path, "%s/menu_state_hold.ppm", outdir); write_ppm(path);
    hold_ms = 0;

    menu_set_mode(MENU_LAUNCHING);
    memset(screen, 0, sizeof screen); menu_render();
    snprintf(path, sizeof path, "%s/menu_state_launching.ppm", outdir); write_ppm(path);

    /*
     * The partial repaint must land exactly what a full one would. Draw the screen with the
     * bar empty, then repaint only the bar's rows with it part-filled, and compare against a
     * full render of that same state.
     */
    menu_set_mode(MENU_BROWSE);
    menu_select_rom(mqart_get(0)->rom);
    hold_ms = 0;   memset(screen, 0, sizeof screen); menu_render();
    hold_ms = 900; menu_render_range(MENU_HOLD_BAR_Y0, MENU_HOLD_BAR_Y1);
    static uint16_t partial[GFX_H][GFX_W];
    memcpy(partial, screen, sizeof screen);

    memset(screen, 0, sizeof screen); menu_render();     /* full, same state */
    int bad = 0;
    for (int y = 0; y < GFX_H; y++)
        for (int x = 0; x < GFX_W; x++)
            if (partial[y][x] != screen[y][x]) bad++;
    printf("partial repaint vs full: %s (%d pixels differ)\n", bad ? "MISMATCH" : "identical", bad);

    printf("wrote %d marquee screens plus 3 states to %s\n", n, outdir);
    return bad ? 2 : 0;
}
