#pragma once
#include <OpenKNX.h>
#include <Arduino.h>
#include <SPI.h>

// SPI1 Konfiguration für den externen Flash (Pins für RP2040)
#define W25Q128_SPI_PORT SPI1
#define W25Q128_CS_PIN 13   // Chip Select Pin
#define W25Q128_CLK_PIN 10  // Clock Pin
#define W25Q128_MISO_PIN 11 // MISO Pin
#define W25Q128_MOSI_PIN 12 // MOSI Pin

// #define W25Q128_SPI_PORT SPI1
// #define W25128_FLASH_CS 13 // Chip Select Pin
// #define W25128_FLASH_SCK 10 // Clock Pin
// #define W25128_FLASH_MOSI 11 // MOSI Pin
// #define W25128_FLASH_MISO 12 // MISO Pin
// #define W25128_FLASH_WP 14 // Write Protect Pin
// #define W25128_FLASH_HOLD 15 // Hold Pin

// W25Q128-Befehle
#define CMD_READ_ID 0x9F
#define CMD_WRITE_ENABLE 0x06
#define CMD_WRITE_DISABLE 0x04
#define CMD_READ_DATA 0x03
#define CMD_PAGE_PROGRAM 0x02
#define CMD_SECTOR_ERASE 0x20
#define CMD_CHIP_ERASE 0xC7
#define CMD_READ_STATUS_REG 0x05
#define CMD_WRITE_STATUS_REG 0x01

#define SECTOR_SIZE_W25Q128_4KB 4096          // 4KB
#define PAGE_SIZE_W25Q128_256B 256            // 256 Bytes
#define FLASH_SIZE_W25Q128 (16 * 1024 * 1024) // 16MB (128Mbit) --> 16 x 1024 x 1024 Bytes = 16777216 Bytes (Hex: 0x1000000)

class W25Q128
{
  public:
    W25Q128();
    void begin();
    void enableWrite();
    void disableWrite();
    uint8_t readStatus();
    void waitUntilReady();

    // LittleFS kompatible Funktionen
    int read(uint32_t addr, uint8_t *buffer, size_t size);
    int program(uint32_t addr, const uint8_t *buffer, size_t size);
    int erase(uint32_t addr);
    void chipErase();

    inline int lfs_read(uint32_t addr, uint8_t *buffer, size_t size) { return read(addr, buffer, size); }
    inline void lfs_prog(uint32_t addr, const uint8_t *buffer, size_t size) { program(addr, buffer, size); }
    inline void lfs_erase(uint32_t addr) { erase(addr); }
    inline void lfs_sync() { waitUntilReady(); }

  private:
    void select();
    void deselect();
    void sendCommand(uint8_t cmd);
    uint8_t transfer(uint8_t data);
};