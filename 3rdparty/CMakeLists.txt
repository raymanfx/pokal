add_subdirectory(Pico_ePaper_Code/c)

add_library(littlefs STATIC
    littlefs/lfs.c
    littlefs/lfs.h
    littlefs/lfs_util.c
    littlefs/lfs_util.h)

target_include_directories(littlefs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/littlefs)

add_library(epaper583b STATIC
    Pico_ePaper_Code/c/lib/Config/DEV_Config.c
    Pico_ePaper_Code/c/lib/e-Paper/EPD_5in83b_V2.c)

target_link_libraries(epaper583b
    pico_stdlib
    hardware_spi)

target_include_directories(epaper583b PUBLIC
    Pico_ePaper_Code/c/lib/Config
    Pico_ePaper_Code/c/lib/e-Paper)

add_library(epd_gui STATIC
    Pico_ePaper_Code/c/lib/GUI/GUI_Paint.c)

target_link_libraries(epd_gui
    Fonts
    epaper583b)

target_include_directories(epd_gui PUBLIC
    Pico_ePaper_Code/c/lib/GUI)
