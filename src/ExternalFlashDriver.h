#pragma once

#include "W25Q128.h" // Dein Treiber für externen Flash
#include "lfs.h"

class ExternalFlashDriver
{
  public:
    static lfs_config getLFSConfig()
    {
        static lfs_config cfg = {
            .context = nullptr, // User defined context, we don't need it here

            // Lets set the callbacks for our W25Q128 Flash
            .read = lfs_read,   // read callback for our W25Q128 Flash
            .prog = lfs_prog,   // program callback for our W25Q128 Flash
            .erase = lfs_erase, // erase callback for our W25Q128 Flash
            .sync = lfs_sync,   // sync callback for pur W25Q128 Flash

#ifdef LFS_THREADSAFE
            .lock = nullptr,   // If thread-safety is needed
            .unlock = nullptr, // If thread-safety is needed
#endif

            // Size configuration for the W25Q128 Flash
            .read_size = PAGE_SIZE_W25Q128_256B,                         // Minimale read size
            .prog_size = PAGE_SIZE_W25Q128_256B,                         // Minimale program size
            .block_size = SECTOR_SIZE_W25Q128_4KB,                       // Sector size of the flash device (4KB)
            .block_count = FLASH_SIZE_W25Q128 / SECTOR_SIZE_W25Q128_4KB, // Number of sectors of the flash device

            .block_cycles = 500,                  // Number of write cycles per block
            .cache_size = PAGE_SIZE_W25Q128_256B, // Cache size
            .lookahead_size = 16,                 // Lookahead buffer size
            .compact_thresh = 0,                  // Default compaxt threshold

            // Static buffers for LittleFS (if needed) we don't need them here
            .read_buffer = nullptr,
            .prog_buffer = nullptr,
            .lookahead_buffer = nullptr,

            // Set maximum sizes
            .name_max = 255,   // Max filname length
            .file_max = 0,     // Max number of files open at the same time, 0 for default
            .attr_max = 0,     // Max number of attributes, 0 for default
            .metadata_max = 0, // Max metadata size, 0 for default
            .inline_max = 0    // Max inline data size. 0 for default

#ifdef LFS_MULTIVERSION
            ,
            .disk_version = 0 // default disk version. 0 seems to be the recent version
#endif
        };
        return cfg;
    }
};