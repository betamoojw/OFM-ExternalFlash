#include "W25Q128.h"

W25Q128::W25Q128() {}

void W25Q128::begin() {
    // SPI1 Setup
    W25Q128_SPI_PORT.begin(W25Q128_CLK_PIN, W25Q128_MISO_PIN, W25Q128_MOSI_PIN, W25Q128_CS_PIN);
    pinMode(W25Q128_CS_PIN, OUTPUT);
    deselect();
    
    openknx.logger.log("W25Q128 initialized!");
}

void W25Q128::enableWrite() {
    select();
    sendCommand(CMD_WRITE_ENABLE);
    deselect();
}

void W25Q128::disableWrite() {
    select();
    sendCommand(CMD_WRITE_DISABLE);
    deselect();
}

uint8_t W25Q128::readStatus() {
    select();
    sendCommand(CMD_READ_STATUS_REG);
    uint8_t status = transfer(0x00);
    deselect();
    return status;
}

void W25Q128::waitUntilReady() {
    while (readStatus() & 0x01) { // Status-Bit 0 prüft "Busy"
        delay(1);
    }
}

int W25Q128::read(uint32_t addr, uint8_t *buffer, size_t size) {
    select();
    sendCommand(CMD_READ_DATA);
    transfer((addr >> 16) & 0xFF);
    transfer((addr >> 8) & 0xFF);
    transfer(addr & 0xFF);

    for (size_t i = 0; i < size; i++) {
        buffer[i] = transfer(0x00);
    }
    deselect();
    return 0; // Erfolg
}

int W25Q128::program(uint32_t addr, const uint8_t *buffer, size_t size) {
    size_t pageSize = 256;
    size_t written = 0;

    while (written < size) {
        size_t chunkSize = min(pageSize, size - written);

        enableWrite();
        select();
        sendCommand(CMD_PAGE_PROGRAM);
        transfer((addr >> 16) & 0xFF);
        transfer((addr >> 8) & 0xFF);
        transfer(addr & 0xFF);

        for (size_t i = 0; i < chunkSize; i++) {
            transfer(buffer[written + i]);
        }
        deselect();
        waitUntilReady();
        
        addr += chunkSize;
        written += chunkSize;
    }
    return 0; // Erfolg
}

int W25Q128::erase(uint32_t addr) {
    enableWrite();
    select();
    sendCommand(CMD_SECTOR_ERASE);
    transfer((addr >> 16) & 0xFF);
    transfer((addr >> 8) & 0xFF);
    transfer(addr & 0xFF);
    deselect();
    waitUntilReady();
    return 0; // Erfolg
}

void W25Q128::chipErase() {
    enableWrite();
    select();
    sendCommand(CMD_CHIP_ERASE);
    deselect();
    waitUntilReady();
}

// Private Methoden
void W25Q128::select() {
    digitalWrite(W25Q128_CS_PIN, LOW);
}

void W25Q128::deselect() {
    digitalWrite(W25Q128_CS_PIN, HIGH);
}

void W25Q128::sendCommand(uint8_t cmd) {
    transfer(cmd);
}

uint8_t W25Q128::transfer(uint8_t data) {
    return W25Q128_SPI_PORT.transfer(data);
}