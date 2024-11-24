/**
 * @class W25Q128
 * @brief A sleek and modern interface for the W25Q128 Flash memory chip, tailored for OpenKNX.
 * 
 * This class offers a comprehensive suite of methods to initialize, read, write, and erase the W25Q128 Flash memory chip.
 * It also includes functionalities to enable and disable write access, read the status register, and ensure the Flash memory is ready for operations.
 * Additionally, it provides a method to retrieve the chip ID and a test function to validate the Flash memory's performance.
 * 
 * Utilizing SPI communication, this class efficiently manages the chip select (CS), write protect (WP), and hold (HOLD) pins, 
 * and handles all SPI transactions seamlessly.
 * 
 * Example usage:
 * @code
 * W25Q128 flash;
 * flash.begin();
 * uint8_t data[256];
 * flash.read(0x000000, data, sizeof(data));
 * @endcode
 * 
 * @note Specifically designed for the W25Q128 Flash memory chip, this class is an integral part of the OpenKNX ecosystem.
 * 
 * @author by Erkan Çolak, 2024
 */



#include "W25Q128.h"

W25Q128 *W25Q128::instance = nullptr;

W25Q128::W25Q128() {}

bool W25Q128::begin()
{

    // Initialise the GPIOs
    pinMode(W25Q128_CS_PIN, OUTPUT);      // Set CS pin as output
    digitalWrite(W25Q128_CS_PIN, HIGH);   // Set CS high

    pinMode(W25Q128_WP_PIN, OUTPUT);      // Set WP pin as output
    digitalWrite(W25Q128_WP_PIN, HIGH);   // Set WP high

    pinMode(W25Q128_HOLD_PIN, OUTPUT);    // Set HOLD pin as output
    digitalWrite(W25Q128_HOLD_PIN, HIGH); // Set HOLD high

    // Seteup the SPI Port
    W25Q128_SPI_PORT.setSCK(W25Q128_SCK_PIN); // Set CLK pin
    W25Q128_SPI_PORT.setTX(W25Q128_MOSI_PIN); // Set MOSI pin
    W25Q128_SPI_PORT.setRX(W25Q128_MISO_PIN); // Set MISO pin
    
    W25Q128_SPI_PORT.begin();                 // Start SPI1
    //delay(100);                             // Wait for initialization

    //pinMode(W25Q128_CS_PIN, OUTPUT); // Set CS pin as output
    deselect();                      // Set CS high
    //openknx.logger.log("W25Q128 initialized!");
    instance = this; // Set the static instance pointer
    return true;     // Replace with actual initialization logic
}

/**
 * @brief Enable write access to the Flash memory
 * 
 */
void W25Q128::enableWrite()
{
    select();
    sendCommand(CMD_WRITE_ENABLE);
    deselect();
}

/**
 * @brief Disable write access to the Flash memory
 * 
 */
void W25Q128::disableWrite()
{
    select();
    sendCommand(CMD_WRITE_DISABLE);
    deselect();
}

/**
 * @brief Read the status register of the Flash memory
 * 
 * @return uint8_t the status register value
 */
uint8_t W25Q128::readStatus()
{
    select();
    sendCommand(CMD_READ_STATUS_REG);
    uint8_t status = transfer(0x00);
    deselect();
    return status;
}

/**
 * @brief Wait until the Flash memory is ready
 * 
 */
void W25Q128::waitUntilReady()
{
    while (readStatus() & 0x01) // Check if the Flash is busy
    { 
        delay(1);
    }
}

/**
 * @brief Read data from the Flash memory
 * 
 * @param addr the address to read from
 * @param buffer the buffer to store the read data
 * @param size the size of the data to read
 * @return int 0 if successful
 */
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
    return 0;
}

/**
 * @brief Write data to the Flash memory
 * 
 * @param addr the address to write to
 * @param buffer the buffer with the data to write
 * @param size the size of the data to write
 * @return int 0 if successful
 */
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
    return 0;
}

/**
 * @brief Erase a sector in the Flash memory
 * 
 * @param addr the address of the sector to erase
 * @return int 0 if successful
 */
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

/**
 * @brief Erase the entire Flash memory
 * 
 */
void W25Q128::chipErase()
{
    enableWrite();
    select();
    sendCommand(CMD_CHIP_ERASE);
    deselect();
    waitUntilReady();
}

// Private Methods

/**
 * @brief Select the Flash memory
 * 
 */
void W25Q128::select()
{   
    digitalWrite(W25Q128_CS_PIN, LOW);
    W25Q128_SPI_PORT.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0)); // SPI-Settings: 8MHz, MSB first, Mode 0
}

/**
 * @brief Deselect the Flash memory
 * 
 */
void W25Q128::deselect()
{
    W25Q128_SPI_PORT.endTransaction(); // End SPI transaction
    digitalWrite(W25Q128_CS_PIN, HIGH);
}

/**
 * @brief Send a command to the Flash memory
 * 
 * @param cmd the command to send
 */
void W25Q128::sendCommand(uint8_t cmd)
{
    transfer(cmd);
}

/**
 * @brief Transfer data to the Flash memory
 * 
 * @param data the data to transfer
 * @return uint8_t the read data
 */
uint8_t W25Q128::transfer(uint8_t data)
{
    return W25Q128_SPI_PORT.transfer(data);
}

/**
 * @brief Read the ID of the Flash memory
 * 
 * @return ChipID the ChipID struct with the manufacturerID, memoryType and capacity
 */
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
 *        WARNING: This function will overwrite data in the given block! Default is block 0.
 *
 * @return true, if the data was written, read and the data matches
 * @return false, if the data was not written, read or the data does not match
 */
bool W25Q128::Test_BlockWriteRead(uint8_t startBlock)
{
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
