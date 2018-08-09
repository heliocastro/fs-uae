set(LIBFSEMU_SOURCES
    src/audio/alsa-audio.c
    src/audio/audio.c
    src/audio/dummy-audio.c
    src/audio/openal-audio.c
    src/audio/sdl-audio.c
    src/base.c
    src/conf.c
    src/data.c
    src/emu/actions.c
    src/emu/cef.cpp
    src/emu/dialog.c
    src/emu/emu.c
    src/emu/font.c
    src/emu/hud.c
    src/emu/input.c
    src/emu/keynames.c
    src/emu/menu.c
    src/emu/netplay.c
    src/emu/render.c
    src/emu/scanlines.c
    src/emu/texture.c
    src/emu/theme.c
    src/emu/util.c
    src/emu/video.c
    src/emu/video_buffer.c
    src/emu/video_options.c
    src/emu/xml_shader.c
    src/eventlog.c
    src/filesys.c
    src/image.c
    src/input/input-actions.c
    src/input/input.c
    src/input/legacy-input.c
    src/input/sdl-input.c
    src/inifile.c
    src/init.c
    src/log.c
    src/malloc.c
    src/ml/input.c
    src/ml/keyboard.c
    src/ml/macosx.c
    src/ml/ml.c
    src/ml/ml_internal.h
    src/ml/mouse.c
    src/ml/opengl.c
    src/ml/rawinput.c
    src/ml/render.c
    src/ml/sdl.c
    src/ml/util.c
    src/ml/video.c
    src/ml/video.c
    src/ml/video_mode.c
    src/ml/video_sync.c
    src/ml/windows.c
    src/ml/x11.c
    src/opengl.c
    src/path.c
    src/ref.c
    src/thread.c
    src/time.c
    src/video/dummy-video.c
    src/video/legacy-video.c
    src/video/opengl-render.c
    src/video/render.c
    src/video/sdl-video.c
    src/video/sdl-video-common.c
    src/video/sdl-video-software.c
    src/video/video.c
    src/video/video-buffer.c
)

add_library(fsemu ${LIBFSEMU_SOURCES})
add_library(fsuae::fsemu ALIAS fsemu)

target_link_libraries(fsemu
    PNG::PNG
    fsuae::manymouse
    )

target_include_directories(fsemu
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PNG_INCLUDE_DIRS}>
        $<BUILD_INTERFACE:${SDL2_INCLUDE_DIRS}>
    )

if(GLIB2_FOUND)
    target_compile_definitions(fsemu PRIVATE "USE_GLIB")
    target_include_directories(fsemu PRIVATE ${GLIB2_INCLUDE_DIRS})
endif()

if(OPENGL_FOUND)
    target_compile_definitions(fsemu PRIVATE "USE_OPENGL")
endif()

if(PNG_FOUND)
    target_compile_definitions(fsemu PRIVATE "USE_PNG")
endif()

if(SDL2_FOUND)
    target_compile_definitions(fsemu PUBLIC "USE_SDL2")
endif()

# if(CMAKE_USE_PTHREADS_INIT)
#     target_compile_definitions(fsemu PRIVATE "USE_PTHREADS")
# endif()

set_target_properties(fsemu
    PROPERTIES
        LINK_FLAGS "-Wl,--as-needed ${link_flags}"
        POSITION_INDEPENDENT_CODE ON
)

