# music/splash.mid is embedded when present. No file, no music, no build error.
set(MIDI "${CMAKE_CURRENT_LIST_DIR}/../../music/splash.mid")
set(EMBEDS "")
if(EXISTS ${MIDI})
    set(EMBEDS ${MIDI})
endif()

idf_component_register(
    SRCS "src/ay8910.c" "src/chiptune.c"
    INCLUDE_DIRS "include"
    REQUIRES audio_hal
    EMBED_FILES ${EMBEDS}
)

if(EXISTS ${MIDI})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE HAVE_SPLASH_MIDI=1)
    message(STATUS "chiptune: embedding music/splash.mid")
else()
    message(STATUS "chiptune: no music/splash.mid - splash will be silent")
endif()
