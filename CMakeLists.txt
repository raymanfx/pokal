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

# 3rd party components
add_subdirectory(3rdparty)

add_executable(pokal
        main.c
        dhcpserver/dhcpserver.c
        dnsserver/dnsserver.c
        repl.c
        repl_fs.c
        fs.c
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
        littlefs
        )

# PICO_CMAKE_CONFIG: PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS, Maximum number of milliseconds to wait during initialization for a CDC connection from the host (negative means indefinite) during initialization, default=0, group=pico_stdio_usb
target_compile_definitions(pokal PRIVATE
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000)

# enable usb output, disable uart output
pico_enable_stdio_usb(pokal 1)
pico_enable_stdio_uart(pokal 0)

pico_add_extra_outputs(pokal)
