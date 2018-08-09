cmake_minimum_required(VERSION 3.0.2)
project(fs-uae
    VERSION 2.9.7
    LANGUAGES C CXX
    )

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(FSUAESystemDefinitions)
include(FSUAEConfigureLegacy)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Options
set(WITH_PEARPC ON CACHE BOOL "Enable pearpc library")

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL REQUIRED)

find_package(PNG REQUIRED)

find_package(PkgConfig REQUIRED)

find_package(SDL2 REQUIRED)

find_package(Threads)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)

pkg_check_modules(MPEG2 REQUIRED libmpeg2)

# Global compile definitions
add_definitions(-DUAE -DFSUAE)

add_subdirectory(manymouse)
add_subdirectory(libfsemu)
add_subdirectory(src)
