cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_examples C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        )
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

add_executable(pokal
        main.c
        dhcpserver/dhcpserver.c
        dnsserver/dnsserver.c
        )

target_include_directories(pokal PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts
        ${CMAKE_CURRENT_LIST_DIR}/dhcpserver
        ${CMAKE_CURRENT_LIST_DIR}/dnsserver
        )

target_link_libraries(pokal
        pico_cyw43_arch_lwip_threadsafe_background
        pico_stdlib
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(pokal 1)
pico_enable_stdio_uart(pokal 0)

pico_add_extra_outputs(pokal)
