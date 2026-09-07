/*
 * input.cpp - menu controls for the MINIMAME launcher.
 *   tilt (QMI8658)      -> browse left/right, one game per detent
 *   BOOT button (GPIO9) -> HOLD to pick the highlighted game (see INPUT_SELECT_HOLD_MS)
 *   PWR button (GPIO18) -> short: re-level the neutral pose; long (1 s): power off
 *
 * The detent is the important part. Raw tilt would rip through sixteen games in
 * half a second, so a step only fires when roll crosses NAV_ON_DEG, and no
 * further step can fire until roll falls back inside NAV_OFF_DEG. Holding past
 * the threshold auto-repeats, slowly at first.
 */
#include "input.h"
#include "qmi8658.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "input";

#define PIN_BTN_BOOT      GPIO_NUM_9
#define PIN_BTN_PWR       GPIO_NUM_18
#define PIN_BAT_EN        GPIO_NUM_15    /* hold HIGH or the medal cuts its own power */

#define PWR_LONG_PRESS_US 1000000
#define IMU_PERIOD_US       16000        /* ~60 Hz is plenty for a menu */

#define NAV_ON_DEG          18.0f        /* cross this to step */
#define NAV_OFF_DEG          8.0f        /* fall back inside this to re-arm */
#define NAV_REPEAT_FIRST_US 450000
#define NAV_REPEAT_NEXT_US  220000

static bool    imu_ok, have_neutral;
static float   neutral_roll;
static int64_t imu_last_us;

static int     armed = 1;                /* may a step fire? */
static int     held_dir;                 /* -1, 0, +1 while past threshold */
static int64_t held_since, last_repeat;
static nav_t   pending_nav = NAV_NONE;

static bool    boot_was_down, hold_consumed, pending_hold;
static int64_t boot_down_since;
static int     hold_ms;
static bool    pwr_was_down;
static int64_t pwr_down_since;

/*
 * The direction of gravity within the panel's plane: rolling the medal left or right rotates
 * it, which is exactly the axis we want for browsing.
 *
 * Returns false when that angle cannot be trusted. Lying flat on a desk, gravity points
 * straight out of the screen, the in-plane component is near zero and its atan2 is noise - and
 * that is where the medal is at power-on, next to a USB cable. A neutral captured there is a
 * centre nobody was holding, and the carousel then either runs away or refuses to move.
 */
static bool read_roll(float *deg)
{
    int16_t ax, ay, az;
    qmi8658_read_accel(&ax, &ay, &az);
    float in_plane = sqrtf((float)ax * ax + (float)ay * ay);
    *deg = atan2f((float)ay, (float)ax) * 57.2958f;
    return in_plane > 1.2f * fabsf((float)az);      /* held up, not lying down */
}

static float wrap_deg(float d)
{
    while (d >  180.0f) d -= 360.0f;
    while (d < -180.0f) d += 360.0f;
    return d;
}

/* Returns false if the medal is not being held up; nothing is captured and the next poll
 * tries again. */
static bool capture_neutral(void)
{
    float roll;
    if (!imu_ok || !read_roll(&roll)) return false;
    neutral_roll = roll;
    have_neutral = true;
    armed = 1; held_dir = 0;
    ESP_LOGI(TAG, "neutral roll %.1f deg", neutral_roll);
    return true;
}

void input_init(void)
{
    gpio_config_t bat = {};
    bat.pin_bit_mask = 1ULL << PIN_BAT_EN;
    bat.mode = GPIO_MODE_OUTPUT;
    gpio_config(&bat);
    gpio_set_level(PIN_BAT_EN, 1);

    gpio_config_t io = {};
    io.pin_bit_mask = (1ULL << PIN_BTN_BOOT) | (1ULL << PIN_BTN_PWR);
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io);

    i2c_config_t i2c = {};
    i2c.mode = I2C_MODE_MASTER;
    i2c.sda_io_num = GPIO_NUM_8;
    i2c.scl_io_num = GPIO_NUM_7;
    i2c.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2c.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2c.master.clk_speed = 100000;
    i2c_param_config(I2C_NUM_0, &i2c);
    esp_err_t err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_LOGW(TAG, "I2C init failed: %s", esp_err_to_name(err));

    imu_ok = qmi8658_init();
    if (imu_ok) {
        vTaskDelay(pdMS_TO_TICKS(120));   /* let it settle before levelling */
        /* this will refuse while the medal is still lying down; input_poll keeps trying */
        capture_neutral();
    } else {
        ESP_LOGW(TAG, "no IMU - browse with the BOOT button only");
    }
}

bool input_imu_ok(void) { return imu_ok; }

void input_poll(void)
{
    int64_t now = esp_timer_get_time();
    bool boot = gpio_get_level(PIN_BTN_BOOT) == 0;
    bool pwr  = gpio_get_level(PIN_BTN_PWR) == 0;

    /* BOOT is a hold, not a press. Track how long it has been down so the menu can
     * draw a progress bar, and fire once when it crosses the threshold. */
    if (boot && !boot_was_down) { boot_down_since = now; hold_consumed = false; }
    if (boot) {
        hold_ms = (int)((now - boot_down_since) / 1000);
        if (!hold_consumed && hold_ms >= INPUT_SELECT_HOLD_MS) {
            pending_hold  = true;
            hold_consumed = true;
        }
    } else {
        hold_ms = 0;
    }
    boot_was_down = boot;

    /* PWR: short press re-levels, long press kills battery rail */
    if (pwr && !pwr_was_down) pwr_down_since = now;
    if (pwr && now - pwr_down_since >= PWR_LONG_PRESS_US) {
        ESP_LOGI(TAG, "power off");
        gpio_set_level(PIN_BAT_EN, 0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    if (!pwr && pwr_was_down && now - pwr_down_since < 400000) capture_neutral();
    pwr_was_down = pwr;

    if (!imu_ok || now - imu_last_us < IMU_PERIOD_US) return;
    imu_last_us = now;

    float raw;
    /* nothing is captured or acted on until the medal is actually being held up */
    if (!read_roll(&raw)) return;
    if (!have_neutral && !capture_neutral()) return;

    float roll = wrap_deg(raw - neutral_roll);
    int dir = (roll >= NAV_ON_DEG) ? +1 : (roll <= -NAV_ON_DEG) ? -1 : 0;

    if (dir == 0) {
        if (fabsf(roll) <= NAV_OFF_DEG) armed = 1;   /* back to centre: re-arm */
        held_dir = 0;
        return;
    }

    if (armed && dir != held_dir) {                  /* fresh crossing */
        pending_nav  = (dir > 0) ? NAV_NEXT : NAV_PREV;
        held_dir     = dir;
        held_since   = now;
        last_repeat  = now;
        armed        = 0;
    } else if (dir == held_dir) {                    /* held over: auto-repeat */
        int64_t gap = (now - held_since >= NAV_REPEAT_FIRST_US)
                        ? NAV_REPEAT_NEXT_US : NAV_REPEAT_FIRST_US;
        if (now - last_repeat >= gap) {
            pending_nav = (dir > 0) ? NAV_NEXT : NAV_PREV;
            last_repeat = now;
        }
    }
}

nav_t input_take_nav(void)
{
    nav_t n = pending_nav;
    pending_nav = NAV_NONE;
    return n;
}

bool input_button_down(void)
{
    return gpio_get_level(PIN_BTN_BOOT) == 0;
}

int input_hold_ms(void) { return hold_ms; }

bool input_take_hold(void)
{
    bool h = pending_hold;
    pending_hold = false;
    return h;
}
