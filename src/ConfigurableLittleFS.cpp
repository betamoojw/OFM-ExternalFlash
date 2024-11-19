#include "ConfigurableLittleFS.h"

ConfigurableLittleFS::ConfigurableLittleFS(bool useExternalFlash)
    : useExternalFlash(useExternalFlash) {}

bool ConfigurableLittleFS::begin()
{
    if (useExternalFlash)
    {
        if (!externalFlash.begin())
        {
            return false; // Failed to initialize external flash
        }
        setupExternalConfig();

        // Mount external flash with LittleFS
        int err = lfs_mount(&lfs, &externalConfig);
        if (err)
        {
            lfs_format(&lfs, &externalConfig); // Format if mount fails
            err = lfs_mount(&lfs, &externalConfig);
        }
        return err == 0;
    }
    else
    {
        return LittleFS.begin(); // Use internal LittleFS
    }
}

void ConfigurableLittleFS::setupExternalConfig()
{
    // COnfiguration for LittleFS with the W25Q128 Flash

    externalConfig.context = nullptr; // User defined context, we don't need it here

    // Lets set the callbacks for our W25Q128 Flash
    externalConfig.read = W25Q128::lfs_read;   // read callback for our W25Q128 Flash
    externalConfig.prog = W25Q128::lfs_prog;   // program callback for our W25Q128 Flash
    externalConfig.erase = W25Q128::lfs_erase; // erase callback for our W25Q128 Flash
    externalConfig.sync = W25Q128::lfs_sync;   // sync callback for pur W25Q128 Flash

#ifdef LFS_THREADSAFE
    externalConfig.lock = nullptr;   // If thread-safety is needed
    externalConfig.unlock = nullptr; // If thread-safety is needed
#endif

    // Size configuration for the W25Q128 Flash
    externalConfig.read_size = PAGE_SIZE_W25Q128_256B;                         // Minimale read size
    externalConfig.prog_size = PAGE_SIZE_W25Q128_256B;                         // Minimale program size
    externalConfig.block_size = SECTOR_SIZE_W25Q128_4KB;                       // Sector size of the flash device (4KB)
    externalConfig.block_count = FLASH_SIZE_W25Q128 / SECTOR_SIZE_W25Q128_4KB; // Number of sectors of the flash device

    externalConfig.block_cycles = 500;                  // Number of write cycles per block
    externalConfig.cache_size = PAGE_SIZE_W25Q128_256B; // Cache size
    externalConfig.lookahead_size = 16;                 // Lookahead buffer size
    externalConfig.compact_thresh = 0;                  // Default compaxt threshold

    // Static buffers for LittleFS (if needed) we don't need them here
    externalConfig.read_buffer = nullptr;
    externalConfig.prog_buffer = nullptr;
    externalConfig.lookahead_buffer = nullptr;

    // Set maximum sizes
    externalConfig.name_max = 255;   // Max filname length
    externalConfig.file_max = 0;     // Max number of files open at the same time, 0 for default
    externalConfig.attr_max = 0;     // Max number of attributes, 0 for default
    externalConfig.metadata_max = 0; // Max metadata size, 0 for default
    externalConfig.inline_max = 0;   // Max inline data size. 0 for default

#ifdef LFS_MULTIVERSION
    externalConfig.disk_version = 0; // default disk version. 0 seems to be the recent version
#endif
}

bool ConfigurableLittleFS::format()
{
    if (useExternalFlash)
    {
        return lfs_format(&lfs, &externalConfig) == 0;
    }
    else
    {
        return LittleFS.format();
    }
}

File ConfigurableLittleFS::open(const char *path, const char *mode)
{
    return LittleFS.open(path, mode);
}

bool ConfigurableLittleFS::remove(const char *path)
{
    return LittleFS.remove(path);
}

bool ConfigurableLittleFS::exists(const char *path)
{
        return LittleFS.exists(path);
}
