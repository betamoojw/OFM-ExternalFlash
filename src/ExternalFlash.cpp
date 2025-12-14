#ifdef EXTERNAL_FLASH_MODULE
/**
 * @class ExternalFlash
 * @brief Interface for managing external flash memory in the OpenKNX ecosystem.
 *
 * This class provides methods to initialize, format, and perform various file operations on external flash memory.
 * It supports file and directory creation, removal, reading, writing, renaming, moving, and copying. Additionally,
 * it offers file statistics and filesystem information, designed for seamless integration with the W25Q128 Flash
 * memory chip.
 *
 * @note Designed to work with the W25Q128 Flash memory chip and integrates seamlessly with the OpenKNX ecosystem.
 *       Should be compatible with other external flash memory chips. Check the datasheet for compatibility.
 *
 * @copyright Copyright (c) 2024 Erkan Çolak - (Licensed under GNU GPL v3.0)
 */

#include "ExternalFlash.h"
#if defined(ARDUINO_ARCH_RP2040)
ExternalFlash extFlashModule; // External flash module instance

/**
 * @brief Construct a new External Flash:: External Flash object
 */
ExternalFlash::ExternalFlash() : _extFlashLfs(FSImplPtr(nullptr)),     // Initialize the LittleFS object
                                 _SpiFlashInit(false), _mounted(false) // Initialize the flags
{
}

/**
 * @brief Destroy the External Flash:: External Flash object
 */
ExternalFlash::~ExternalFlash()
{
    // ToDo delete the new instance of extLittleFSImpl
    if (_mounted)
    {
        _extFlashLfs.end(); // Rleases the internal resources
    }
}

/**
 * @brief Initialize the ExternalFlash module. Check if we can initialize
 *        the external flash using SPI and set the flag _SpiFlashInit
 */
void ExternalFlash::init()
{
    // Initialize the external flash
    logDebugP("Initializing external flash");
    if (!_SpiFlash.begin()) // Initialize the external flash
    {
        logInfoP("Failed to initialize external spi flash");
    }
    else
    {
        logInfoP("External spi flash initialized");
        _SpiFlashInit = true;
    }
}

/**
 * @brief Setup the ExternalFlash module. Setup the Filesystem (_extFlashLfs) to the external spi flash
 * @param configured, true if OpenKNX is configured
 */
void ExternalFlash::setup(bool configured)
{
    // Create the external LittleFS instance with the start and end address of the flash memory
    logDebugP("Setting up the spi flash instance");

    // Get now the ext_LittleFSImpl instance from the FS object
    logDebugP("Initializing LFS Settings");
    setupExternalConfig();                              // ToDo EC: Make a configuration wrapper for the external flash settings
    uint8_t extFlash_FS_start_addr = 0x000;             // Start address of the W25q128 flash memory
    uint32_t extFLash_FS_end_addr = FLASH_SIZE_W25Q128; // End address of the W25q128 flash memory

    ext_littlefs_impl::ext_LittleFSImpl *extLittleFSImpl =
        new ext_littlefs_impl::ext_LittleFSImpl(
            &extFlash_FS_start_addr, extFLash_FS_end_addr, // Start and end address of the flash memory
            PAGE_SIZE_W25Q128_256B,                        // Size of a page in flash
            SECTOR_SIZE_W25Q128_4KB, 16);                  // Size of a block in flash, Maximum number of open file descriptors

    logDebugP("Setting up external ext_LittleFS configuration");
    if (extLittleFSImpl->setLFSConfig(_extFlashLfsConfig))
    {
        _extFlashLfs = FS(FSImplPtr(extLittleFSImpl)); // Set the external flash filesystem
        logDebugP("Mounting external flash with ext_LittleFS");
        if (!_extFlashLfs.begin()) // Mount the external flash
        {
            logErrorP("Failed to mount external flash with ext_LittleFS. Formatting...");
            if (_extFlashLfs.format()) // Format the external flash it it fails to mount
            {
                if (_extFlashLfs.begin()) // Retry to mount the external flash
                {
                    logInfoP("External  flash formatted with ext_LittleFS");
                    _mounted = true;
                }
                else
                    logErrorP("Failed to mount external flash with ext_LittleFS after formatting");
            }
            else
                logErrorP("Failed to format external flash with ext_LittleFS");
        }
        else
        {
            logInfoP("External flash mounted with ext_LittleFS");
            _mounted = true;
        }
    }
    else
        logErrorP("Failed to set external flash configuration");

    if (!_mounted)
    {
        // Failed to mount external flash with ext_LittleFS. Delete new instance of extLittleFSImpl
        delete extLittleFSImpl;
    }
    else
    {
        // Set the time callback for the external flash, this is optional.
        _extFlashLfs.setTimeCallback([]() -> time_t { return openknx.time.getLocalTime().toTime_t(); });
    }
}

/**
 * @brief loop for the ExternalFlash module
 * @param configured, true if OpenKNX is configured
 */
void ExternalFlash::loop(bool configured)
{
    /*NOOP*/
}

/**
 * @brief Process GroupObjects
 * @param ko, GroupObject to process
 */
void ExternalFlash::processInputKo(GroupObject &ko)
{
    /*NOOP*/
}

void ExternalFlash::showHelp()
{
    openknx.console.printHelpLine("efc", "External Flash Control Module. Use 'efc ?' for more.");
}

/**
 * @brief Process the console commands for the ExternalFlash module.
 * @param command, the command to process
 * @param diagnose, if true, will not process the command
 */
bool ExternalFlash::processCommand(const std::string command, bool diagnose)
{
    bool bRet = false;
    if ((!diagnose) && command.compare(0, 4, "efc ") == 0)
    {
        bRet = true; // Ok, we are in efc command!
        if (command.compare(4, 1, "?") == 0 || command.compare(4, 4, "help") == 0 || command.length() == 4)
        {
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("============================= Help: External Flash Control ============================="); // 88 characters
            openknx.logger.color(0);
            openknx.logger.log("Command(s)               Description");
            openknx.console.printHelpLine("efc info", "Get information about the external flash");
            openknx.console.printHelpLine("efc add /<f>", "Add a folder/file to the external flash");
            openknx.console.printHelpLine("efc rm /<f>", "Remove a file from the external flash");
            openknx.console.printHelpLine("efc cat /<f>", "Read a file from the external flash");
            openknx.console.printHelpLine("efc echo /<file> <text>", "Append content to a file in the external flash");
            openknx.console.printHelpLine("efc mv /<src> /<targt>", "Rename/ or Move a file or folder");
            openknx.console.printHelpLine("efc cp /<src> /<targt>", "Copy a file in the external flash");
            openknx.console.printHelpLine("efc mkdir /<name>", "Create a directory in the external flash");
            openknx.console.printHelpLine("efc rmdir /<name>", "Remove a directory from the external flash");
            openknx.console.printHelpLine("efc ls /<path>", "Short list files in a directory in the external flash");
            openknx.console.printHelpLine("efc ll /<path>", "List files in a directory in the external flash with details");
            openknx.console.printHelpLine("efc format", "ATTENTION: Will Format the external flash");
            openknx.console.printHelpLine("efc test", "Creating files, folders, writing and reading files");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("----------------------------------------------------------------------------------------"); // 88 characters
            openknx.logger.color(0);
            openknx.logger.end();
        }
        else if (command.compare(4, 4, "info") == 0)
        {
            FSInfo info;
            if (_extFlashLfs.info(info))
            {
                logInfoP("External Flash Info:");
                logInfoP(String("Total Bytes: " + String((unsigned long)info.totalBytes, DEC)).c_str());
                logInfoP(String("Used Bytes: " + String((unsigned long)info.usedBytes, DEC)).c_str());
                logInfoP(String("Block Size: " + String((unsigned long)info.blockSize, DEC)).c_str());
                logInfoP(String("Page Size: " + String((unsigned long)info.pageSize, DEC)).c_str());
                logInfoP(String("Max Open Files: " + String((unsigned long)info.maxOpenFiles, DEC)).c_str());
            }
            else
            {
                logErrorP("Failed to get external flash info");
                bRet = false;
            }
        }
        else if (command.compare(4, 6, "format") == 0)
        {
            if (_extFlashLfs.format())
            {
                logInfoP("External Flash formatted");
            }
            else
            {
                logErrorP("Failed to format external flash");
                bRet = false;
            }
        }
        else if (command.compare(4, 4, "test") == 0)
        {
            logInfoP("External Flash Test:");
            logInfoP("Writing 'test.txt' with message 'Hello, External LittleFS!' to external LittleFS...");
            File file = open("/test.txt", "w");
            if (file)
            {
                file.print("Hello, External LittleFS!");
                file.close();
                logInfoP("Reading 'test.txt' from external LittleFS...");
                file = open("/test.txt", "r");
                if (file)
                {
                    String content = file.readString();
                    logInfoP(String("Read from external LittleFS: " + content).c_str());
                    file.close();
                }
                else
                {
                    logErrorP("Failed to read 'test.txt' from external LittleFS.");
                    bRet = false;
                }
            }
            else
            {
                logErrorP("Failed to write 'test.txt' to external LittleFS.");
                bRet = false;
            }

            logInfoP("Creating files, folders, writing and reading files. This may take a while. Please wait...");
            // Create multiple directories and files with random content
            const char *dirNames[] = {"/", "/documents", "/projects", "/backups", "/logs", "/temp", "/trash", "/downloads", "VeryLongFolderNameVeryLongFolderNameVeryLongFolderName"};
            const char *fileNames[] = {"HAL9000.txt", "Odyssey.txt", "Discovery.txt", "Jupiter.txt", "Monolith.txt",
                                       "Bowman.txt", "Poole.text", "Floyd.ini", "Curnow.fcg", "Chandra.dat",
                                       "Whitehead.bin", "Hunter.uf2", "Kimball.tmp", "Tanya", "Victor.logfile", "VeryLongFileNameVeryLongFileNameVeryLongFileName.log"};

            for (int i = 0; i < sizeof(dirNames) / sizeof(dirNames[0]); ++i) // Create 8 directories
            {
                mkdir(dirNames[i]);
                for (int j = 0; j < sizeof(fileNames) / sizeof(fileNames[0]); ++j) // Create 15 files in each directory
                {
                    String fileName = String(dirNames[i]) + "/" + fileNames[j];
                    File file = open(fileName.c_str(), "w");
                    logDebugP(String("Creating file: " + fileName).c_str());
                    if (file) // Write now Random content to the file. Which is a random numbers and the HAL9000 message
                    {
                        logDebugP(String("Writing test content to file: " + fileName).c_str());
                        String content = "HAL9000: I'm sorry, Dave. I'm afraid I can't do that.\n";
                        for (int k = 0; k < random(1, 100); ++k)
                        {
                            file.print(content);
                            for (int k = 0; k < random(1, 100); ++k)
                            {
                                file.print(String(random(0, 1000)) + "\n");
                            }
                        }
                        file.close();
                    }
                }
            }
            logInfoP("Files and folders created. To show the list of files use 'efc ls /' or 'efc ll /'");
        }
        else if (command.compare(4, 4, "add ") == 0)
        {
            String fileName = command.substr(8).c_str();
            if (fileName[0] != '/')
            {
                fileName = "/" + fileName;
            }

            if (fileName.length() > 0 && fileName.length() <= 255 && fileName[0] == '/' && createFile(fileName.c_str()))
            {
                logInfoP(String("File created: " + fileName).c_str());
            }
            else
            {
                logErrorP("File name is invalid");
                bRet = false;
            }
        }
        else if (command.compare(4, 3, "rm ") == 0)
        {
            String fileName = command.substr(7).c_str();
            if (fileName[0] != '/')
            {
                fileName = "/" + fileName;
            }
            if (fileName.length() > 0 && remove(fileName.c_str()))
            {
                logInfoP(String("File removed: " + fileName).c_str());
            }
            else
            {
                logErrorP("Failed to remove file");
                bRet = false;
            }
        }
        else if (command.compare(4, 4, "cat ") == 0)
        {
            String fileName = command.substr(9).c_str();
            if (fileName[0] != '/')
            {
                fileName = "/" + fileName;
            }
            if (fileName.length() > 0)
            {
                uint8_t buffer[256];
                size_t bytesRead = read(fileName.c_str(), buffer, sizeof(buffer));
                if (bytesRead > 0)
                {
                    logInfoP(String("Read from file: " + String((char *)buffer)).c_str());
                }
                else
                {
                    logErrorP("Failed to read file");
                    bRet = false;
                }
            }
            else
            {
                logErrorP("Invalid file name");
                bRet = false;
            }
        }
        else if (command.compare(4, 5, "echo ") == 0)
        {
            // Get the file name which begins with / and ends with space
            String fileName = command.substr(9, command.find(' ', 9) - 9).c_str();

            // After the file name, get the content to write to the file it will begin after the space of the file name
            String content = command.substr(command.find(' ', 9) + 1).c_str();
            if (fileName.length() > 0 && content.length() > 0)
            {
                File file = open(fileName.c_str(), "a");
                if (!file)
                {
                    file = open(fileName.c_str(), "w");
                    if (!file)
                    {
                        logErrorP("Failed to create file");
                        bRet = false;
                    }
                    else
                    {
                        logInfoP(String("File created: " + fileName).c_str());
                        file.println(content);
                        file.close();
                        logInfoP(String("Appended to file: " + fileName).c_str());
                    }
                }
                else
                {
                    file.println(content);
                    file.close();
                    logInfoP(String("Appended to file: " + fileName).c_str());
                }
            }
            else
            {
                logErrorP("Invalid file name or content");
                bRet = false;
            }
        }
        else if (command.compare(4, 3, "mv ") == 0)
        {
            String oldName = command.substr(7, command.find(' ', 7) - 7).c_str();
            if (oldName[0] != '/')
            {
                oldName = "/" + oldName;
            }
            String newName = command.substr(command.find(' ', 7) + 1).c_str();
            if (newName[0] != '/')
            {
                newName = "/" + newName;
            }
            if (oldName.length() > 0 && newName.length() > 0 && rename(oldName.c_str(), newName.c_str()))
            {
                logInfoP(String("Renamed from " + oldName + " to " + newName).c_str());
            }
            else
            {
                logErrorP("Failed to rename %s to %s", oldName.c_str(), newName.c_str());
                bRet = false;
            }
        }
        else if (command.compare(4, 6, "mkdir ") == 0)
        {
            String dirName = command.substr(11).c_str();
            if (dirName[0] != '/')
            {
                dirName = "/" + dirName;
            }
            if (dirName.length() > 0 && mkdir(dirName.c_str()))
            {
                logInfoP(String("Directory created: " + dirName).c_str());
            }
            else
            {
                logErrorP("Failed to create directory");
                bRet = false;
            }
        }
        else if (command.compare(4, 6, "rmdir ") == 0)
        {
            String dirName = command.substr(11).c_str();
            if (dirName[0] != '/')
            {
                dirName = "/" + dirName;
            }
            if (dirName.length() > 0 && rmdir(dirName.c_str()))
            {
                logInfoP(String("Directory removed: " + dirName).c_str());
            }
            else
            {
                logErrorP("Failed to remove directory");
                bRet = false;
            }
        }
        else if (command.compare(4, 3, "ll ") == 0)
        {
            logInfoP("External Flash Files:");
            String path = command.substr(7).c_str();
            std::vector<String> files = ls(path.length() == 0 ? "/" : path.c_str());
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("========================== External Flash Control File system =========================="); // 80 characters
            openknx.logger.log("----------------------------------------------------------------------------------------");
            openknx.logger.log("Name                                      | Size (bytes) | Type   | Created             ");
            openknx.logger.log("----------------------------------------------------------------------------------------"); // 88 characters
            openknx.logger.color(0);
            if (files.size() == 0)
            {
                openknx.logger.log("..(empty)");
            }
            uint64_t totalSize = 0;
            u_int64_t CountedFoders = 0;
            // Separate files and directories
            std::vector<String> directories, regularFiles;
            for (String file : files)
            {
                FSStat stat;
                if (Statistics(file.c_str(), stat))
                {
                    if (stat.isDir)
                    {
                        directories.push_back(file);
                        CountedFoders++;
                    }
                    else
                    {
                        regularFiles.push_back(file);
                    }
                }
                else
                {
                    logErrorP(String("Failed to get stats for: " + file).c_str());
                }
            }

            // Process directories first
            for (String dir : directories)
            {
                FSStat stat;
                if (Statistics(dir.c_str(), stat))
                {
                    const String type = "Dir";
                    char formattedTime[25];
                    strftime(formattedTime, sizeof(formattedTime), "%H:%M:%S %d.%m.", localtime(&stat.ctime));
                    sprintf(formattedTime + strlen(formattedTime), "%02d", (localtime(&stat.ctime)->tm_year + 1900) % 100);

                    const String name = "[" + ((dir.length() > 37) ? dir.substring(0, 36) + "..." : dir) + "]";
                    totalSize += stat.size;
                    openknx.logger.logWithValues("%-41s | %-12s | %-6s | %-20s",
                                                 name.c_str(),
                                                 String(/*getSize(dir.c_str())*/ "").c_str(), type.c_str(), formattedTime);
                }
                else
                {
                    // Stat failed. Show only the directory name
                    openknx.logger.logWithValues("%-41s | %-12s | %-6s | %-20s",
                                                 String((dir.length() > 41) ? dir.substring(0, 38) + "..." : dir).c_str(),
                                                 "N/A", "Dir", "N/A");
                }
            }

            // Process regular files
            for (String file : regularFiles)
            {
                FSStat stat;
                if (Statistics(file.c_str(), stat))
                {
                    const String type = "File";
                    char formattedTime[25];
                    strftime(formattedTime, sizeof(formattedTime), "%H:%M:%S %d.%m.", localtime(&stat.ctime));
                    sprintf(formattedTime + strlen(formattedTime), "%02d", (localtime(&stat.ctime)->tm_year + 1900) % 100);
                    totalSize += stat.size;
                    openknx.logger.logWithValues("%-41s | %-12s | %-6s | %-20s",
                                                 // file.c_str(),
                                                 String((file.length() > 41) ? file.substring(0, 38) + "..." : file).c_str(),
                                                 String(stat.size).c_str(), type.c_str(), formattedTime);
                }
                else
                {
                    // Stat failed. Show only the file name
                    openknx.logger.logWithValues("%-41s | %-12s | %-6s | %-20s",
                                                 String((file.length() > 41) ? file.substring(0, 38) + "..." : file).c_str(),
                                                 "N/A", "File", "N/A");
                }
            }
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("----------------------------------------------------------------------------------------"); // 88 characters
            // const String totalFiles = String("Total files: " + String((unsigned long)files.size(), DEC) + " | Total size: " + String((unsigned long)totalSize, DEC) + " bytes");
            openknx.logger.logWithValues("%-20s %-20s | %-12s",
                                         String("Folders: " + String((unsigned long)CountedFoders, DEC)).c_str(),
                                         String("Files: " + String((unsigned long)(files.size() - CountedFoders), DEC)).c_str(),
                                         String("Size: " + String((unsigned long)totalSize, DEC) + " bytes").c_str());
            openknx.logger.log("----------------------------------------------------------------------------------------"); // 88 characters

            FSInfo info;
            if (_extFlashLfs.info(info))
            {
                const float usedPercentage = (float)info.usedBytes / info.totalBytes * 100.0f;
                openknx.logger.log("Total Storage extFlash: ");
                openknx.logger.logWithValues("Used: %-20s [%-50s] %.1f%%", // Used space
                                             String((unsigned long)info.usedBytes, DEC).c_str(),
                                             String("==============================================").substring(0, (int)(usedPercentage / 2)).c_str(), // Bar length
                                             usedPercentage);

                openknx.logger.logWithValues("Free: %-20s [%-50s] %.1f%%", // Free space
                                             String((unsigned long)(info.totalBytes - info.usedBytes), DEC).c_str(),
                                             String("==============================================").substring(0, (50 - (int)(usedPercentage / 2))).c_str(), // Bar length
                                             100.0f - usedPercentage);
            }
            openknx.logger.log("----------------------------------------------------------------------------------------"); // 88 characters

            openknx.logger.color(0);
            openknx.logger.end();
        }
        else if (command.compare(4, 3, "ls ") == 0)
        {
            String path = command.substr(7).c_str();
            logInfoP("External Flash Files:");
            std::vector<String> files = ls(path.length() == 0 ? "/" : path.c_str());
            for (String file : files)
            {
                logInfoP(file.c_str());
            }
        }
        else
        {
            logErrorP("Invalid command. Use 'efs ?' for help.");
            bRet = false;
        }
    }
    return bRet;
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
bool ExternalFlash::Statistics(const String path, FSStat &stat)
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
#endif //
#endif // #ifdef EXTERNAL_FLASH_MODULE