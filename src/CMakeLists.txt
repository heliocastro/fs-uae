
include_directories(
    include
)

add_subdirectory(
    od-fs
)

# Generators
add_executable(gen_genlinetoscr
    genlinetoscr.cpp
    $<TARGET_OBJECTS:od-fs>
    )

    target_link_libraries(gen_genlinetoscr PUBLIC fsuae::fsemu)

