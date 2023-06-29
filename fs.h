// Pico SDK
#include "hardware/flash.h"

#include "lfs.h"

// reserve 1M for program and 1M for the FS
#define FS_SIZE 1024 * 1024
#define FS_START PICO_FLASH_SIZE_BYTES - FS_SIZE

int rp2040_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
int rp2040_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
int rp2040_erase(const struct lfs_config *c, lfs_block_t block);
int rp2040_sync(const struct lfs_config *c);

// global FS context
extern lfs_t lfs;

// configuration of the filesystem is provided by this struct
static const struct lfs_config lfs_cfg = {
    // block device operations
    .read  = rp2040_read,
    .prog  = rp2040_prog,
    .erase = rp2040_erase,
    .sync  = rp2040_sync,

    // block device configuration
    // see https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash/include/hardware/flash.h
    .read_size = FLASH_PAGE_SIZE,
    .prog_size = FLASH_PAGE_SIZE,
    .block_size = FLASH_BLOCK_SIZE,
    // see https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards/pico.h
    .block_count = FS_SIZE / FLASH_BLOCK_SIZE,
    .cache_size = FLASH_PAGE_SIZE,
    .lookahead_size = 16,
    .block_cycles = 500,
};
