idf_component_register(
    SRCS "main.cpp" "menu.cpp" "input.cpp" "games.cpp" "splash.c" "battery.c"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_partition app_update nvs_flash display imu mqart gfx medalboot fest chiptune audio_hal esp_adc
)
