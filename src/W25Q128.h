#pragma once
/**
 * @file        W25Q128.h
 * @brief       Interface for external flash memory management in OpenKNX.
 * @author      Erkan Çolak
 * @version     1.0.0
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 */

 #if defined(ARDUINO_ARCH_RP2040)
#include <Arduino.h>
#include <FS.h>
#include <OpenKNX.h>
#include <SPI.h>
#ifdef ARDUINO_ARCH_RP2040
    #include "../lib/littlefs/lfs.h"
#elif defined(ARDUINO_ARCH_ESP32)
    #include <LittleFS.h>
#else
    #error "Unsupported platform for W25Q128!"
#endif

#ifndef OKNXHW_REG2_EXTFLASH_SPI_SCK // SPI Clock pin
//ERROR_REQUIRED_DEFINE(OKNXHW_REG2_EXTFLASH_SPI_SCK);
#define OKNXHW_REG2_EXTFLASH_SPI_SCK = 18; // Default SCK pin for ESP32
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_INST // SPI instance (e.g., SPI, SPI1)
#define OKNXHW_REG2_EXTFLASH_SPI_INST = SPI; // Default SPI instance
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_MOSI // MOSI pin
#define OKNXHW_REG2_EXTFLASH_SPI_MOSI = 23; // Default MOSI pin for ESP32
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_MISO // MISO pin
#define OKNXHW_REG2_EXTFLASH_SPI_MISO = 19; // Default MISO pin for ESP32
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_CS // Chip Select (CS) pin
#define OKNXHW_REG2_EXTFLASH_SPI_CS = 5; // Default CS pin for ESP32
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_WP // Write Protect (WP) pin
#define OKNXHW_REG2_EXTFLASH_SPI_WP = 4; // Default WP pin for ESP32
#endif
#ifndef OKNXHW_REG2_EXTFLASH_SPI_HOLD // Hold pin
#define OKNXHW_REG2_EXTFLASH_SPI_HOLD = 2; // Default HOLD pin for ESP32
#endif

// W25Q128 SPecific Commands (could be compatible with other W25Qxx Flash Chips)
#define CMD_READ_ID 0x9F          // Read ID Command
#define CMD_WRITE_ENABLE 0x06     // Write Enable Command
#define CMD_WRITE_DISABLE 0x04    // Write Disable Command
#define CMD_READ_DATA 0x03        // Read Data Command
#define CMD_PAGE_PROGRAM 0x02     // Page Program Command
#define CMD_SECTOR_ERASE 0x20     // Sector Erase Command
#define CMD_CHIP_ERASE 0xC7       // Chip Erase Command
#define CMD_READ_STATUS_REG 0x05  // Read Status Register Command
#define CMD_WRITE_STATUS_REG 0x01 // Write Status Register Command

#define SECTOR_SIZE_W25Q128_4KB 4096          // The sector size of the W25Q128 Flash Chip is 4KB!
#define PAGE_SIZE_W25Q128_256B 256            // Page size of the W25Q128 Flash Chip is 256 Bytes
#define FLASH_SIZE_W25Q128 (16 * 1024 * 1024) // 16MB (128Mbit) --> 16 x 1024 x 1024 Bytes = 16777216 Bytes (Hex: 0x1000000)

// Chip ID struct, to store the manufacturerID, memoryType and capacity
struct ChipID
{
    uint8_t manufacturerID; // Manufacturer ID, should be 0xEF for Winbond
    uint8_t memoryType;     // Memory Type, should be 0x40 for W25Q128
    uint8_t capacity;       // Capacity, should be 0x18 for 16Mbit
};

class W25Q128
{
  public:
    W25Q128();
    bool begin();
    void enableWrite();
    void disableWrite();
    uint8_t readStatus();
    void waitUntilReady();

    // LittleFS kompatible Funktionen
    int read(uint32_t addr, uint8_t *buffer, size_t size);
    int program(uint32_t addr, const uint8_t *buffer, size_t size);
    int erase(uint32_t addr);
    void chipErase();

#ifdef ARDUINO_ARCH_RP2040
    // LittleFS Callbacks for RP2040
    inline static int lfs_read(const struct lfs_config *c, lfs_block_t block,
                               lfs_off_t off, void *buffer, lfs_size_t size)
    {
        uint32_t addr = block * c->block_size + off;
        return instance->read(addr, static_cast<uint8_t *>(buffer), size);
    }

    inline static int lfs_prog(const struct lfs_config *c, lfs_block_t block,
                               lfs_off_t off, const void *buffer, lfs_size_t size)
    {
        uint32_t addr = block * c->block_size + off;
        return instance->program(addr, static_cast<const uint8_t *>(buffer), size);
    }

    inline static int lfs_erase(const struct lfs_config *c, lfs_block_t block)
    {
        uint32_t addr = block * c->block_size;
        return instance->erase(addr);
    }

    inline static int lfs_sync(const struct lfs_config *c)
    {
        return 0;
    }
#endif

    bool Test_BlockWriteRead(uint8_t startBlock = 0);
    ChipID readID();
    static W25Q128 *instance;

  private:
    void select();
    void deselect();
    void sendCommand(uint8_t cmd);
    uint8_t transfer(uint8_t data);
};
#endif // ARDUINO_ARCH_RP2040