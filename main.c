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

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }

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
