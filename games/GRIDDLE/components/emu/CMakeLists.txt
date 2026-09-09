# Platform-independent Burger Time emulation core (shared with the host harness in ../../host)
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../core)
idf_component_register(
    SRCS "${CORE_DIR}/btime.c" "${CORE_DIR}/btime_sound.c" "${CORE_DIR}/btime_video.c" "${CORE_DIR}/ay8910.c"
    INCLUDE_DIRS "${CORE_DIR}"
    LDFRAGMENTS "linker.lf"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -fjump-tables -ftree-switch-conversion)
