#include <stdio.h>
#include <string.h>

#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdio_usb.h"
#include "repl.h"

#include "repl_reset.h"

static void repl_reset_flash(const char *path);
static void repl_reset_usb(const char *path);

static struct repl_cmd LUT[] = {
    {
        .cmd = "flash",
        .handler = repl_reset_flash,
    },
    {
        .cmd = "usb",
        .handler = repl_reset_usb,
    },
};

void repl_reset(const char *line) {
    for (int i = 0; i < sizeof(LUT) / sizeof(LUT[0]); i++) {
        if (strncmp(line, LUT[i].cmd, strlen(LUT[i].cmd)) == 0) {
            LUT[i].handler(line + strlen(LUT[i].cmd) + 1);
            return;
        }
    }

    printf("< reset: unknown cmd: %s\n", line);
}

static void repl_reset_flash(const char *path) {
    printf("< reset to flash\n");
    watchdog_reboot(0, 0, PICO_STDIO_USB_RESET_RESET_TO_FLASH_DELAY_MS);
}

static void repl_reset_usb(const char *path) {
    printf("< reset to usb download\n");
    reset_usb_boot(0, 0);
}
