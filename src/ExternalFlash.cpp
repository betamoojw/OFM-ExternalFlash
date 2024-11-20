#include "ExternalFlash.h"

ExternalFlash extFlashModule; // External flash module instance

ExternalFlash::ExternalFlash() : _extFlashLfs(FSImplPtr(nullptr)),
                                 _SpiFLashInitialized(false), _mounted(false)
{
}

ExternalFlash::~ExternalFlash()
{
    // ToDo delete the new instance of extLittleFSImpl
    if (_mounted)
    {
        _extFlashLfs.end(); // Rleases the internal resources
    }
}

void ExternalFlash::init()
{
    logDebugP("init()");
    // Initialize the external flash
    logDebugP("Initializing external flash");
    if (!_SpiFlash.begin())
    {
        logInfoP("Failed to initialize external spi flash");
    }
    else
    {
        logInfoP("External spi flash initialized");
        _SpiFLashInitialized = true;
    }
}

void ExternalFlash::setup(bool configured)
{
    logDebugP("setup()");
    // Create the external LittleFS instance with the start and end address of the flash memory
    logDebugP("Setting up spi flash ext_LittleFS instance");

    // Get now the ext_LittleFSImpl instance from the FS object
    logDebugP("Initializing LFS Settings");
    setupExternalConfig();                              // ToDo EC: Make a configuration wrapper for the external flash settings
    uint8_t extFlash_FS_start_addr = 0x000;             // Start address of the W25q128 flash memory
    uint32_t extFLash_FS_end_addr = FLASH_SIZE_W25Q128; // End address of the W25q128 flash memory

    ext_littlefs_impl::ext_LittleFSImpl *extLittleFSImpl = new ext_littlefs_impl::ext_LittleFSImpl(&extFlash_FS_start_addr, extFLash_FS_end_addr, PAGE_SIZE_W25Q128_256B, SECTOR_SIZE_W25Q128_4KB, 16);

    logDebugP("Setting up external ext_LittleFS configuration");
    if (extLittleFSImpl->setLFSConfig(_extFlashLfsConfig))
    {
        _extFlashLfs = FS(FSImplPtr(extLittleFSImpl));
        logDebugP("Mounting external flash with ext_LittleFS");
        if (!_extFlashLfs.begin())
        {
            logErrorP("Failed to mount external flash with ext_LittleFS. Formatting...");
            if (_extFlashLfs.format())
            {
                logInfoP("External  flash formatted with ext_LittleFS");
                _mounted = true;
            }
            else
            {
                logErrorP("Failed to format external flash with ext_LittleFS");
            }
        }
        else
        {
            logInfoP("External flash mounted with ext_LittleFS");
            _mounted = true;
        }
    }
    else
    {
        logErrorP("Failed to set external flash configuration");
    }

    if (!_mounted)
    {
        // Failed to mount external flash with ext_LittleFS. Delete new instance of extLittleFSImpl
        delete extLittleFSImpl;
    }
}

/**
 * @brief loop for the ExternalFlash module
 *
 * @param configured, true if OpenKNX is configured
 */
void ExternalFlash::loop(bool configured)
{
    /*NOOP*/
}

/**
 * @brief Process GroupObjects
 *
 * @param ko, GroupObject to process
 */
void ExternalFlash::processInputKo(GroupObject &ko)
{
    /*NOOP*/
}

/**
 * @brief Formats the file system.
 *
 * This function formats the file system, either using external flash or internal LittleFS.
 *
 * @return True if the format is successful, false otherwise.
 */
bool ExternalFlash::format()
{
    return _extFlashLfs.format();
}

/**
 * @brief Retrieves file system information.
 *
 * This function retrieves information about the file system.
 *
 * @param info A reference to an FSInfo object to store the information.
 * @return True if the information is successfully retrieved, false otherwise.
 */
bool ExternalFlash::info(FSInfo &info)
{
    return _extFlashLfs.info(info);
}

/**
 * @brief Retrieves file statistics.
 *
 * This function retrieves statistics about a file at the specified path.
 *
 * @param path The path to the file.
 * @param stat A reference to an FSStat object to store the statistics.
 * @return True if the statistics are successfully retrieved, false otherwise.
 */
bool ExternalFlash::stat(const String path, FSStat &stat)
{
    return _extFlashLfs.stat(path, &stat);
}

/**
 * @brief Opens a file.
 *
 * This function opens a file at the specified path with the given mode.
 *
 * @param path The path to the file.
 * @param mode The mode to open the file in.
 * @return A File object representing the opened file.
 */
File ExternalFlash::open(const char *path, const char *mode)
{
    return _extFlashLfs.open(path, mode);
}

/**
 * @brief Creates a file.
 *
 * This function creates a file at the specified path.
 *
 * @param path The path to the file.
 * @return True if the file is successfully created, false otherwise.
 */
bool ExternalFlash::createFile(const char *path)
{
    File file = _extFlashLfs.open(path, "w");
    if (!file)
    {
        return false;
    }
    file.close();
    return true;
}

/**
 * @brief Removes a file.
 *
 * This function removes a file at the specified path.
 *
 * @param path The path to the file.
 * @return True if the file is successfully removed, false otherwise.
 */
bool ExternalFlash::remove(const char *path)
{

    return _extFlashLfs.remove(path);
}

/**
 * @brief Checks if a file exists.
 *
 * This function checks if a file exists at the specified path.
 *
 * @param path The path to the file.
 * @return True if the file exists, false otherwise.
 */
bool ExternalFlash::exists(const char *path)
{
    return _extFlashLfs.exists(path);
}

/**
 * @brief Reads data from a file.
 *
 * This function reads data from a file at the specified path into the given buffer.
 *
 * @param path The path to the file.
 * @param buffer The buffer to read data into.
 * @param size The size of the buffer.
 * @return The number of bytes read from the file.
 */
size_t ExternalFlash::read(const char *path, uint8_t *buffer, size_t size)
{
    File file = _extFlashLfs.open(path, "r");
    if (!file)
    {
        return 0;
    }
    size_t bytesRead = file.read(buffer, size);
    file.close();
    return bytesRead;
}

/**
 * @brief Writes data to a file.
 *
 * This function writes data from the given buffer to a file at the specified path.
 *
 * @param path The path to the file.
 * @param buffer The buffer containing the data to write.
 * @param size The size of the data to write.
 * @return The number of bytes written to the file.
 */
size_t ExternalFlash::write(const char *path, const uint8_t *buffer, size_t size)
{
    File file = _extFlashLfs.open(path, "w");
    if (!file)
    {
        return 0;
    }
    size_t bytesWritten = file.write(buffer, size);
    file.close();
    return bytesWritten;
}

/**
 * @brief Renames a file.
 *
 * This function renames a file from the old path to the new path.
 *
 * @param oldPath The current path to the file.
 * @param newPath The new path to the file.
 * @return True if the file is successfully renamed, false otherwise.
 */
bool ExternalFlash::rename(const char *oldPath, const char *newPath)
{
    return _extFlashLfs.rename(oldPath, newPath);
}

/**
 * @brief Creates a directory.
 *
 * This function creates a directory at the specified path.
 *
 * @param path The path to the directory.
 * @return True if the directory is successfully created, false otherwise.
 */
bool ExternalFlash::mkdir(const char *path)
{
    return _extFlashLfs.mkdir(path);
}

/**
 * @brief Creates a directory.
 *
 * This function creates a directory at the specified path.
 *
 * @param path The path to the directory.
 * @return True if the directory is successfully created, false otherwise.
 */
bool ExternalFlash::createDir(const char *path)
{
    return _extFlashLfs.mkdir(path);
}

/**
 * @brief Removes a directory.
 *
 * This function removes a directory at the specified path.
 *
 * @param path The path to the directory.
 * @return True if the directory is successfully removed, false otherwise.
 */
bool ExternalFlash::rmdir(const char *path)
{
    return _extFlashLfs.rmdir(path);
}

/**
 * @brief Lists files in a directory.
 *
 * This function lists the files in a directory at the specified path.
 *
 * @param path The path to the directory.
 * @return True if the directory is successfully listed, false otherwise.
 */
std::vector<String> ExternalFlash::ls(const char *path)
{
    std::vector<String> fileList;
    File dir = _extFlashLfs.open(path, "r");
    if (!dir || !dir.isDirectory())
    {
        return fileList;
    }
    File file = dir.openNextFile();
    while (file)
    {
        fileList.push_back(file.name());
        file = dir.openNextFile();
    }
    return fileList;
}

/**
 * @brief Moves a file or directory.
 *
 * This function moves a file or directory from the old path to the new path.
 *
 * @param oldPath The current path to the file or directory.
 * @param newPath The new path to the file or directory.
 * @return True if the file or directory is successfully moved, false otherwise.
 */
bool ExternalFlash::move(const char *oldPath, const char *newPath)
{
    return rename(oldPath, newPath);
}

/**
 * @brief Copies a file.
 *
 * This function copies a file from the source path to the destination path.
 *
 * @param srcPath The source path to the file.
 * @param destPath The destination path to the file.
 * @return True if the file is successfully copied, false otherwise.
 */
bool ExternalFlash::copyFile(const char *srcPath, const char *destPath)
{
    File srcFile = _extFlashLfs.open(srcPath, "r");
    if (!srcFile)
    {
        return false;
    }
    File destFile = _extFlashLfs.open(destPath, "w");
    if (!destFile)
    {
        srcFile.close();
        return false;
    }
    size_t size = srcFile.size();
    uint8_t *buffer = new uint8_t[size];
    srcFile.read(buffer, size);
    destFile.write(buffer, size);
    delete[] buffer;
    srcFile.close();
    destFile.close();
    return true;
}

/**
 * @brief Copies a directory.
 *
 * This function copies a directory and its contents from the source path to the destination path.
 *
 * @param srcPath The source path to the directory.
 * @param destPath The destination path to the directory.
 * @return True if the directory is successfully copied, false otherwise.
 */
bool ExternalFlash::copyDir(const char *srcPath, const char *destPath)
{
    File srcDir = _extFlashLfs.open(srcPath, "r");
    if (!srcDir || !srcDir.isDirectory())
    {
        return false;
    }
    if (!_extFlashLfs.mkdir(destPath))
    {
        return false;
    }
    File file = srcDir.openNextFile();
    while (file)
    {
        String srcFilePath = String(srcPath) + "/" + file.name();
        String destFilePath = String(destPath) + "/" + file.name();
        if (file.isDirectory())
        {
            if (!copyDir(srcFilePath.c_str(), destFilePath.c_str()))
            {
                return false;
            }
        }
        else
        {
            if (!copyFile(srcFilePath.c_str(), destFilePath.c_str()))
            {
                return false;
            }
        }
        file = srcDir.openNextFile();
    }
    return true;
}

/**
 * @brief Retrieves the size of a file or directory.
 *
 * This function retrieves the size of a file or directory at the specified path.
 *
 * @param path The path to the file or directory.
 * @return The size of the file or directory in bytes.
 */
size_t ExternalFlash::getSize(const char *path)
{
    File file = _extFlashLfs.open(path, "r");
    if (!file)
    {
        return 0;
    }
    size_t size = file.size();
    file.close();
    return size;
}

/**
 * @brief Retrieves the creation time of a file or directory.
 *
 * This function retrieves the creation time of a file or directory at the specified path.
 *
 * @param path The path to the file or directory.
 * @return The creation time of the file or directory.
 */
time_t ExternalFlash::getCreationTime(const char *path)
{
    FSStat stat;
    if (_extFlashLfs.stat(path, &stat))
    {
        return stat.ctime;
    } 
    else
    {
        return 0;
    }
}

/**
 * @brief Retrieves the modification time of a file or directory.
 *
 * This function retrieves the modification time of a file or directory at the specified path.
 *
 * @param path The path to the file or directory.
 * @return The modification time of the file or directory.
 */
time_t ExternalFlash::getModificationTime(const char *path)
{
    return getAccessTime(path);
}

/**
 * @brief Retrieves the access time of a file or directory.
 *
 * This function retrieves the access time of a file or directory at the specified path.
 *
 * @param path The path to the file or directory.
 * @return The access time of the file or directory.
 */
time_t ExternalFlash::getAccessTime(const char *path)
{
    FSStat stat;
    if (_extFlashLfs.stat(path, &stat))
    {
        return stat.atime;
    } 
    else
    {
        return 0;
    }
}

/**
 * @brief Sets up the external flash configuration.
 *
 * This function configures the external flash settings for LittleFS.
 */
void ExternalFlash::setupExternalConfig() // ToDo EC: Make a configuration wrapper for the external flash settings
{
    // COnfiguration for LittleFS with the W25Q128 Flash

    _extFlashLfsConfig.context = nullptr; // User defined context, we don't need it here

    // Lets set the callbacks for our W25Q128 Flash
    _extFlashLfsConfig.read = W25Q128::lfs_read;   // read callback for our W25Q128 Flash
    _extFlashLfsConfig.prog = W25Q128::lfs_prog;   // program callback for our W25Q128 Flash
    _extFlashLfsConfig.erase = W25Q128::lfs_erase; // erase callback for our W25Q128 Flash
    _extFlashLfsConfig.sync = W25Q128::lfs_sync;   // sync callback for pur W25Q128 Flash

#ifdef LFS_THREADSAFE
    _extFlashLfsConfig.lock = nullptr;   // If thread-safety is needed
    _extFlashLfsConfig.unlock = nullptr; // If thread-safety is needed
#endif

    // Size configuration for the W25Q128 Flash
    _extFlashLfsConfig.read_size = PAGE_SIZE_W25Q128_256B;                         // Minimale read size
    _extFlashLfsConfig.prog_size = PAGE_SIZE_W25Q128_256B;                         // Minimale program size
    _extFlashLfsConfig.block_size = SECTOR_SIZE_W25Q128_4KB;                       // Sector size of the flash device (4KB)
    _extFlashLfsConfig.block_count = FLASH_SIZE_W25Q128 / SECTOR_SIZE_W25Q128_4KB; // Number of sectors of the flash device

    _extFlashLfsConfig.block_cycles = 500;                  // Number of write cycles per block
    _extFlashLfsConfig.cache_size = PAGE_SIZE_W25Q128_256B; // Cache size
    _extFlashLfsConfig.lookahead_size = 16;                 // Lookahead buffer size
    _extFlashLfsConfig.compact_thresh = 0;                  // Default compaxt threshold

    // Static buffers for LittleFS (if needed) we don't need them here
    _extFlashLfsConfig.read_buffer = nullptr;
    _extFlashLfsConfig.prog_buffer = nullptr;
    _extFlashLfsConfig.lookahead_buffer = nullptr;

    // Set maximum sizes
    _extFlashLfsConfig.name_max = 255;   // Max filname length
    _extFlashLfsConfig.file_max = 0;     // Max number of files open at the same time, 0 for default
    _extFlashLfsConfig.attr_max = 0;     // Max number of attributes, 0 for default
    _extFlashLfsConfig.metadata_max = 0; // Max metadata size, 0 for default
    _extFlashLfsConfig.inline_max = 0;   // Max inline data size. 0 for default

#ifdef LFS_MULTIVERSION
    _extFlashLfsConfig.disk_version = 0; // default disk version. 0 seems to be the recent version
#endif
}