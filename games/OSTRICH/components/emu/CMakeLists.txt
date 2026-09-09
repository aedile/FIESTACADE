# Platform-independent Joust emulation core (shared with the host harness in ../../host)
set(CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../core)
idf_component_register(
    SRCS "${CORE_DIR}/joust.c" "${CORE_DIR}/joust_video.c" "${CORE_DIR}/joust_sound.c"
    INCLUDE_DIRS "${CORE_DIR}"
    LDFRAGMENTS "linker.lf"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -O3 -fjump-tables -ftree-switch-conversion -Wno-misleading-indentation)
