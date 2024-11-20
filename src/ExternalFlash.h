#pragma once
#include "OpenKNX.h"
#include "W25Q128.h"
#include "ext_LittleFS.h"

#define ExternalFlash_Display_Name "ExternalFlash" // Display name
#define ExternalFlash_Display_Version "0.0.1"      // Display version

// Extend LittleFS to support dynamic configuration for external flash
class ExternalFlash : public OpenKNX::Module
{
  public:
    
    //OpenKNX required functions
    void init();                                   // Initialize the ExternalFlash module
    void setup(bool configured) override;          // Setup the Filesystem (_extFlashLfs)
    void loop(bool configured) override;           // Loop for the ExternalFlash module
    void processInputKo(GroupObject& ko) override; // Process GroupObjects

    // Constructor for ExternalFlash
    ExternalFlash();
    ~ExternalFlash();

    // Filesystem operations
    inline bool isMounted() { return _mounted; } // Check if the filesystem is mounted
    bool format(); // Format the filesystem
    bool info(FSInfo &info); // Get filesystem information
    bool stat(const String path, FSStat &stat); // Get file statistics

    File open(const char *path, const char *mode); // Open a file
    bool createFile(const char *path); // Create a file
    bool remove(const char *path); // Remove a file
    bool exists(const char *path); // Check if a file exists
    size_t read(const char *path, uint8_t *buffer, size_t size); // Read data from a file
    size_t write(const char *path, const uint8_t *buffer, size_t size); // Write data to a file
    bool rename(const char *oldPath, const char *newPath); // Rename a file

    // Folder/Directory operations
    bool mkdir(const char *path); // Create a directory
    bool createDir(const char *path); // Create a directory
    bool rmdir(const char *path); // Remove a directory
    std::vector<String> ls(const char *path); // List files in a directory

    // Move/Copy operations
    bool move(const char *oldPath, const char *newPath); // Move a file or directory
    bool copyFile(const char *srcPath, const char *destPath); // Copy a file
    bool copyDir(const char *srcPath, const char *destPath); // Copy a directory

    // File information
    size_t getSize(const char *path); // Get the size of a file or directory
    time_t getCreationTime(const char *path); // Get the creation time of a file or directory
    time_t getModificationTime(const char *path); // Get the modification time of a file or directory
    time_t getAccessTime(const char *path); // Get the access time of a file or directory

    // OpenKNX Module interface
    inline const std::string name() { return ExternalFlash_Display_Name; }
    inline const std::string version() { return ExternalFlash_Display_Version; }

  private:
    W25Q128 _SpiFlash;     // Instance of external flash
    lfs_config _extFlashLfsConfig; // Configuration for external flash
    FS _extFlashLfs;           // LittleFS object for external flash
    bool _SpiFLashInitialized; // Flag to check if the external flash is initialized
    bool _mounted;             // Flag to check if the filesystem is mounted

    void setupExternalConfig();
}; // class ExternalFlash

extern ExternalFlash extFlashModule; // External flash module instance


