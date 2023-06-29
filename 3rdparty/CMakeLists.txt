add_library(littlefs STATIC
    littlefs/lfs.c
    littlefs/lfs.h
    littlefs/lfs_util.c
    littlefs/lfs_util.h)

target_include_directories(littlefs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/littlefs)
