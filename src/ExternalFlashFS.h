#pragma once

#include <FS.h>
#include "lfs.h"  // LittleFS Core
#include "ExternalFlashDriver.h" // Treiber für den externen Flash (W25Q128)

class ExternalFlashFS {
public:
    static bool begin(bool formatOnFail = false) {
        lfs_config cfg = ExternalFlashDriver::getLFSConfig();
        if (lfs_mount(&lfs, &cfg) != 0) {
            if (formatOnFail) {
                return (lfs_format(&lfs, &cfg) == 0 && lfs_mount(&lfs, &cfg) == 0);
            }
            return false;
        }
        return true;
    }

    static File open(const char* path, const char* mode) {
        // Implementiere die Open-Funktion mit LittleFS und externer Flash
        // Dummy-Funktion als Beispiel
        if (!lfs_file_open(&lfs, &file, path, LFS_O_RDWR | LFS_O_CREAT)) {
            return File();  // Hier kannst du eine passende Implementierung hinzufügen
        }
    }

    static bool exists(const char* path) {
        // Beispiel für Datei-Check im externen Flash
        lfs_info info;
        return lfs_stat(&lfs, path, &info) == 0;
    }

private:
    static lfs_t lfs;
    static lfs_file_t file;
};