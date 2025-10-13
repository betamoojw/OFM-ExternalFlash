# OFM-ExternalFlash

## Description

The `OFM-ExternalFlash` module is a library for managing and utilizing external flash memory (W25Q128). It provides a simple interface for reading, writing, and erasing data on external flash memory devices.

## Features

- **Read Data**: Allows reading data from the external flash memory.
- **Write Data**: Supports writing data to the external flash memory.
- **Erase Data**: Provides functions to erase data blocks in the external flash memory.
- **Initialization**: Offers functions for initializing and configuring the external flash memory.

## Usage

To use the `OFM-ExternalFlash` library, include it in your project and use the provided functions to manage the external flash memory. 

### PlatformIO Configuration
To enable the external flash module, add the following definitions to your `platformio.ini`:
```ini
; Enable the external flash module
-D EXTERNAL_FLASH_MODULE

; Disable the global external LittleFS
-D NO_GLOBAL_EXT_LITTLEFS
```

## Example

```cpp
#include "ExternalFlash.h"

#ifdef EXTERNAL_FLASH_MODULE
  openknx.addModule(12, extFlashModule);
#endif

if (extFlashModule.isMounted()) {
  openknx.logger.log("External Flash mounted. Successfully initialized external LittleFS.");
} else {
  openknx.logger.log("Failed to initialize external LittleFS.");
}

// Check the ExternalFlash class for detailed functions like read, write, erase, etc.
// You can use the console to test the functions. See the Command Overview.
```

### Features

- **Command Processing**: Execute commands for file and directory management, including:
  - Displaying filesystem information (`efc info`)
  - Formatting the flash memory (`efc format`)
  - File read/write tests (`efc test`)
  - Listing files and directories (`efc ls`)
  - Creating directories (`efc mkdir`)
  - Copying, moving, and deleting files (`efc cp`, `efc mv`, `efc rm`)
  - Displaying file contents (`efc cat`)
- **Filesystem Statistics**: Displays usage statistics, directory contents, and metadata for individual files.

### Command Overview

| Command    | Description                              |
|------------|------------------------------------------|
| `efc info` | Display filesystem information           |
| `efc format` | Format the external flash memory        |
| `efc ls`   | List files and directories               |
| `efc mkdir`| Create a new directory                   |
| `efc cp`   | Copy a file                              |
| `efc mv`   | Move or rename a file                    |
| `efc rm`   | Remove a file or directory               |
| `efc cat`  | Display file contents                    |
| `efc test` | Perform read/write tests on flash memory |
