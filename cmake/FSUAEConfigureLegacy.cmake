include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckStructHasMember)
include(CheckCSourceCompiles)

#check_struct_has_member("struct stat" st_rdev "sys/stat.h"
check_struct_has_member("struct stat" st_blocks "sys/stat.h" HAVE_STRUCT_STAT_ST_BLOCKS LANGUAGE C)
check_struct_has_member("struct stat" st_mtim.tv_nsec "sys/stat.h" HAVE_STRUCT_STAT_ST_MTIM_TV_NSEC LANGUAGE C)
check_struct_has_member("struct stat" st_mtimespec.tv_nsec "sys/stat.h"
    HAVE_STRUCT_STAT_ST_MTIMESPEC_TV_NSEC LANGUAGE C)
check_struct_has_member("struct stat" st_mtime_nsec "sys/stat.h" HAVE_STRUCT_STAT_ST_MTIME_NSEC LANGUAGE C)

configure_file(${PROJECT_SOURCE_DIR}/cmake/config.h.in ${PROJECT_BINARY_DIR}/config.h)

include_directories(${PROJECT_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)


