#include "fs.h"

// Pico SDK
#include <hardware/sync.h>

// General usage of the RP2040 SDK flash_range_* functions:
// see https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash/include/hardware/flash.h
// and https://github.com/raspberrypi/pico-examples/blob/master/flash/program/flash_program.c

// We need to save and restore the interrupt context before touching the flash.
// see https://github.com/raspberrypi/pico-examples/issues/34
// and https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/

// Global FS context
lfs_t lfs;

int rp2040_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t addr = FS_START + block * c->block_size + off;

    // The RP2040 maps the entire flash memory into the global address space.
    // It puts it right behind the RAM, which means that we have to account for the RAM in order to directly
    // access the flash. The amount of bytes of RAM is XIP_BASE.
    addr += XIP_BASE;

    memcpy(buffer, (const void*)addr, size);

    return 0;
}

int rp2040_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = FS_START + block * c->block_size + off;
    uint32_t ints;

    ints = save_and_disable_interrupts();
    flash_range_program(addr, (const uint8_t*)buffer, size);
    restore_interrupts(ints);

    return 0;
}

int rp2040_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = FS_START + block * c->block_size;
    uint32_t ints;

    ints = save_and_disable_interrupts();
    flash_range_erase(addr, c->block_size);
    restore_interrupts(ints);

    return 0;
}

int rp2040_sync(const struct lfs_config *c) {
    // no HW caches, nothing to do
    return 0;
}
