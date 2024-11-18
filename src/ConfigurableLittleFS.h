#pragma once
#include <LittleFS.h>
#include "ExternalFlashFS.h"  // Unsere Abstraktion für den externen Flash

#define ExternalFlash_Display_Name "ExternalFlash" // Display name
#define ExternalFlash_Display_Version "0.0.1"      // Display version

// Flashtype enum, to define the flash type
enum FlashType {
    INTERNAL_FLASH,
    EXTERNAL_FLASH
};

class ConfigurableLittleFS : public LittleFS {
public:
    // Constructor with the flashType internal flash as default value
    ConfigurableLittleFS(FlashType flashType = INTERNAL_FLASH)
        : flashType(flashType) {}

    // Begin method which is initialised based on the flashType
    bool begin(bool formatOnFail = false) {
        if (flashType == EXTERNAL_FLASH) {
            openknx.logger.log("Initialise the external flash file system");
            return ExternalFlashFS::begin(formatOnFail);
        } else {
            openknx.logger.log("Initialise the default internal flash file system");
            return LittleFS::begin(formatOnFail);
        }
    }

    // Override the open method
    File open(const char* path, const char* mode) {
        if (flashType == EXTERNAL_FLASH) {
            return ExternalFlashFS::open(path, mode);
        } else {
            return LittleFS::open(path, mode);
        }
    }

    // Override the exists method
    bool exists(const char* path) {
        if (flashType == EXTERNAL_FLASH) {
            return ExternalFlashFS::exists(path);
        } else {
            return LittleFS::exists(path);
        }
    }

private:
    FlashType flashType;
};


class ExternalFlash : public OpenKNX::Module {
public:
    ExternalFlash() {}

    void init() override {
        // Initialise the external flash file system
        if (!lfs.begin()) {
            openknx.logger.log("Error mounting LittleFS");
            lfs.format();
            lfs.begin();
        }
    }

    void setup(bool configured) override {
        // Set the external flash file system as default
        openknx.logger.log("Set the external flash file system as default");
        lfs = ConfigurableLittleFS(EXTERNAL_FLASH);
    }

    void loop(bool configured) override {
        // Do nothing in the loop
    }

    inline const std::string name() { return ExternalFlash_Display_Name; }
    inline const std::string version() { return ExternalFlash_Display_Version; }

private:
    ConfigurableLittleFS lfs;
};
