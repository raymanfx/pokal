/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "dhcpserver.h"
#include "dnsserver.h"
#include "httpserver.h"

#include "repl.h"
#include "repl_fs.h"
#include "repl_reset.h"
#include "repl_ups.h"
#include "repl_epaper.h"
#include "fs.h"
#include "route_root.h"
#include "route_led.h"
#include "route_fs.h"
#include "epaper_gui.h"

// Read Eval Print Loop (REPL) command look-up table
static struct repl_cmd REPL_CMDS[] = {
    {
        .cmd = "fs",
        .handler = repl_fs,
    },
    {
        .cmd = "reset",
        .handler = repl_reset,
    },
    {
        .cmd = "ups",
        .handler = repl_ups,
    },
    {
        .cmd = "epaper",
        .handler = repl_epaper,
    },
};

// HTTP routes consisting of a method (e.g. "GET") and a path (e.g. "/")
// NOTE: most specific *must* come first!
static http_route_t HTTP_ROUTES[] = {
    http_route_get_fs,
    http_route_post_fs,
    http_route_get_led,
    http_route_get_root,
};

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

    sleep_ms(1000);

    const uint LED_PIN = 16;
    const uint SWITCH_PIN = 17;

    //set Wifi LED
    gpio_init(LED_PIN);     
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //read external switch
    gpio_init(SWITCH_PIN);
    gpio_set_dir(SWITCH_PIN, GPIO_IN);

    if(gpio_get(SWITCH_PIN)==1){
        gpio_put(LED_PIN, 1);
    }else{
        gpio_put(LED_PIN, 0);
    }

    //first draw after bottup
    epaper_gui_draw();

    // mount the filesystem
    int err = lfs_mount(&lfs, &lfs_cfg);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs, &lfs_cfg);
        lfs_mount(&lfs, &lfs_cfg);
        printf("failed to mount FS, formatting ..\n");
    }

    // read current count
    lfs_file_t file;
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 1;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // print the boot count
    printf("boot_count: %d\n", boot_count);

    // ensure the database is present
    lfs_file_open(&lfs, &file, "db.txt", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_close(&lfs, &file);

    // TODO: read from SPI flash
    const char *ap_name = "picow_test";
    const char *password = "password";

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    http_server_t http_server;
    ip4_addr_t mask;
    IP4_ADDR(ip_2_ip4(&http_server.gw), 192, 168, 4, 1);
    IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &http_server.gw, &mask);

    // Start the dns server
    dns_server_t dns_server;
    dns_server_init(&dns_server, &http_server.gw);

    // Mount HTTP routes
    http_server.routes = HTTP_ROUTES;
    http_server.routes_len = sizeof(HTTP_ROUTES) / sizeof(http_route_t);

    // Start the http server
    if (!http_server_open(&http_server)) {
        printf("failed to open HTTP server\n");
        return 1;
    }

    // Read Eval Print Loop (REPL)
    repl_enter(REPL_CMDS, sizeof(REPL_CMDS) / sizeof(REPL_CMDS[0]));

    http_server_close(&http_server);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();

    // release any resources we were using
    lfs_unmount(&lfs);

    // reboot by using the watchdog to reset to flash
    watchdog_reboot(0, 0, PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS);

    return 0;
}
