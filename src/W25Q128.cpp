#include "W25Q128.h"

W25Q128 *W25Q128::instance = nullptr;

W25Q128::W25Q128() {}

bool W25Q128::begin()
{

     // Initialisierung der GPIOs
    pinMode(W25Q128_CS_PIN, OUTPUT);
    digitalWrite(W25Q128_CS_PIN, HIGH);

    pinMode(W25Q128_WP_PIN, OUTPUT);
    digitalWrite(W25Q128_WP_PIN, HIGH);

    pinMode(W25Q128_HOLD_PIN, OUTPUT);
    digitalWrite(W25Q128_HOLD_PIN, HIGH);

    // SPI1 Setup
    W25Q128_SPI_PORT.setSCK(W25Q128_SCK_PIN); // Set CLK pin
    W25Q128_SPI_PORT.setTX(W25Q128_MOSI_PIN); // Set MOSI pin
    W25Q128_SPI_PORT.setRX(W25Q128_MISO_PIN); // Set MISO pin
    
    W25Q128_SPI_PORT.begin();                 // Start SPI1
    //delay(100);                               // Wait for initialization

    //pinMode(W25Q128_CS_PIN, OUTPUT); // Set CS pin as output
    deselect();                      // Set CS high
    //openknx.logger.log("W25Q128 initialized!");
    instance = this; // Set the static instance pointer
    return true;     // Replace with actual initialization logic
}

void W25Q128::enableWrite()
{
    select();
    sendCommand(CMD_WRITE_ENABLE);
    deselect();
}

void W25Q128::disableWrite()
{
    select();
    sendCommand(CMD_WRITE_DISABLE);
    deselect();
}

uint8_t W25Q128::readStatus()
{
    select();
    sendCommand(CMD_READ_STATUS_REG);
    uint8_t status = transfer(0x00);
    deselect();
    return status;
}

void W25Q128::waitUntilReady()
{
    while (readStatus() & 0x01)
    { // Status-Bit 0 prüft "Busy"
        delay(1);
    }
}

int W25Q128::read(uint32_t addr, uint8_t *buffer, size_t size)
{
    select();
    sendCommand(CMD_READ_DATA);
    transfer((addr >> 16) & 0xFF);
    transfer((addr >> 8) & 0xFF);
    transfer(addr & 0xFF);

    for (size_t i = 0; i < size; i++)
    {
        buffer[i] = transfer(0x00);
    }
    deselect();
    return 0; // Erfolg
}

int W25Q128::program(uint32_t addr, const uint8_t *buffer, size_t size)
{
    size_t pageSize = 256;
    size_t written = 0;

    while (written < size)
    {
        size_t chunkSize = min(pageSize, size - written);

        enableWrite();
        select();
        sendCommand(CMD_PAGE_PROGRAM);
        transfer((addr >> 16) & 0xFF);
        transfer((addr >> 8) & 0xFF);
        transfer(addr & 0xFF);

        for (size_t i = 0; i < chunkSize; i++)
        {
            transfer(buffer[written + i]);
        }
        deselect();
        waitUntilReady();

        addr += chunkSize;
        written += chunkSize;
    }
    return 0; // Erfolg
}

int W25Q128::erase(uint32_t addr)
{
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

void W25Q128::chipErase()
{
    enableWrite();
    select();
    sendCommand(CMD_CHIP_ERASE);
    deselect();
    waitUntilReady();
}

// Private Methoden
void W25Q128::select()
{   
    digitalWrite(W25Q128_CS_PIN, LOW);
    W25Q128_SPI_PORT.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); // SPI-Settings: 8MHz, MSB first, Mode 0
}

void W25Q128::deselect()
{
    W25Q128_SPI_PORT.endTransaction(); // End SPI transaction
    digitalWrite(W25Q128_CS_PIN, HIGH);
}

void W25Q128::sendCommand(uint8_t cmd)
{
    transfer(cmd);
}

uint8_t W25Q128::transfer(uint8_t data)
{
    return W25Q128_SPI_PORT.transfer(data);
}

// Some internal functions to test the W25Q128 Flash
ChipID W25Q128::readID()
{
    select();
    sendCommand(CMD_READ_ID);
    ChipID chipID;
    chipID.manufacturerID = transfer(0x00);
    chipID.memoryType = transfer(0x00);
    chipID.capacity = transfer(0x00);
    deselect();
    return chipID;
}

/**
 * @brief Test function for the W25Q128 Flash, to check if the Flash is working correctly.
 *        We will write some data to the Flash and read it back and compare the data.
 *        WARNING: This function will overwrite data in the Flash memory (BLOCK 0)!
 *
 * @return true, if the data was written, read and the data matches
 * @return false, if the data was not written, read or the data does not match
 */
bool W25Q128::Test_BlockWriteRead(uint8_t startBlock)
{

    // startBlock = 0; Default is 0! Start at block!
    uint8_t writeBuf[256]; // 256 Byte
    uint8_t readBuf[256];  // 256 Byte

    // Write buffer with test data
    for (size_t i = 0; i < sizeof(writeBuf); i++)
    {
        writeBuf[i] = i;
    }

    // Block write
    program(startBlock, writeBuf, sizeof(writeBuf));

    // Block read,
    read(startBlock, readBuf, sizeof(readBuf));

    // Compare
    return memcmp(writeBuf, readBuf, sizeof(writeBuf)) == 0;
}
