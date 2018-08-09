set(MANYMOUSE_SOURCES
    linux_evdev.c
    macosx_hidmanager.c
    macosx_hidutilities.c
    manymouse.c
    windows_wminput.c
    x11_xinput2.c
    )

add_library(manymouse ${MANYMOUSE_SOURCES})
add_library(fsuae::manymouse ALIAS manymouse)

target_include_directories(manymouse SYSTEM
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    )
set_target_properties(manymouse
    PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        LINK_FLAGS "-Wl,--as-needed"
)

