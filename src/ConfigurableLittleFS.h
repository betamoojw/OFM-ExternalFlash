#pragma once
#include "W25Q128.h"
#include <LittleFS.h>

#define ExternalFlash_Display_Name "ExternalFlash" // Display name
#define ExternalFlash_Display_Version "0.0.1"      // Display version

// Extend LittleFS to support dynamic configuration for external flash
class ConfigurableLittleFS
{
  public:
    // Constructor
    ConfigurableLittleFS(bool useExternalFlash = false);

    // Initialize the filesystem
    bool begin();
    bool format();
    File open(const char *path, const char *mode);
    bool remove(const char *path);
    bool exists(const char *path);

    inline const std::string name() { return ExternalFlash_Display_Name; }
    inline const std::string version() { return ExternalFlash_Display_Version; }
    W25Q128 externalFlash;     // Instance of external flash
  private:
    bool useExternalFlash;     // Flag to use external flash
    //W25Q128 externalFlash;     // Instance of external flash
    lfs_t lfs;                 // LittleFS object for external flash
    lfs_config externalConfig; // Configuration for external flash

    void setupExternalConfig(); // Helper to configure external flash
};