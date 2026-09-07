idf_component_register(
    SRCS "main.cpp" "menu.cpp" "input.cpp" "games.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver esp_timer esp_partition app_update nvs_flash display imu mqart gfx medalboot
)
