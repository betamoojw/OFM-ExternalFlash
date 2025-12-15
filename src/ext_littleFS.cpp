/*
    Ext LittleFS.cpp - Wrapper for LittleFS for RP2040
    Copyright (c) 2024 Erkan Çolak. All rights reserved.

    Based on the Wrapper for LittleFS for RP2040 which is
    Copyright (c) 2021 Earle F. Philhower, III.  All rights reserved.

    Based extensively off of the ESP8266 SPIFFS code, which is
    Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef EXTERNAL_FLASH_MODULE
#if defined(ARDUINO_ARCH_RP2040)
#include "ext_LittleFS.h"
#include <hardware/flash.h>

#ifdef USE_TINYUSB
    // For Serial when selecting TinyUSB.  Can't include in the core because Arduino IDE
    // will not link in libraries called from the core.  Instead, add the header to all
    // the standard libraries in the hope it will still catch some user cases where they
    // use these libraries.
    // See https://github.com/earlephilhower/arduino-pico/issues/167#issuecomment-848622174
    #include <Adafruit_TinyUSB.h>
#endif

namespace ext_littlefs_impl
{
    /**
     * @brief Construct a new ext_LittleFSImpl object with full configuration support
     *
     * @param path to the filesystem in flash
     * @param openMode OpenMode, OM_DEFAULT, OM_CREATE, OM_APPEND, OM_TRUNCATE
     * @param accessMode  AM_READ, AM_WRITE, AM_RW (read/write)
     *
     * @return FileImplPtr to the file implementation
     */
    FileImplPtr ext_LittleFSImpl::open(const char *path, OpenMode openMode, AccessMode accessMode)
    {
        if (!_mounted)
        {
            DEBUGV("ext_LittleFSImpl::open() called on unmounted FS\n");
            return FileImplPtr();
        }
        if (!path || !path[0])
        {
            DEBUGV("ext_LittleFSImpl::open() called with invalid filename\n");
            return FileImplPtr();
        }
        if (!ext_LittleFSImpl::pathValid(path))
        {
            DEBUGV("ext_LittleFSImpl::open() called with too long filename\n");
            return FileImplPtr();
        }

        const int flags = _getFlags(openMode, accessMode);
        auto fd = std::make_shared<lfs_file_t>();

        if ((openMode & OM_CREATE) && strchr(path, '/'))
        {
            // For file creation, silently make subdirs as needed.  If any fail,
            // it will be caught by the real file open later on
            char *pathStr = strdup(path);
            if (pathStr)
            {
                // Make dirs up to the final fnamepart
                char *ptr = strchr(pathStr, '/');
                while (ptr)
                {
                    *ptr = 0;
                    lfs_mkdir(&_lfs, pathStr);
                    *ptr = '/';
                    ptr = strchr(ptr + 1, '/');
                }
            }
            free(pathStr);
        }

        time_t creation = 0;
        if (_timeCallback && (openMode & OM_CREATE))
        {
            // O_CREATE means we *may* make the file, but not if it already exists.
            // See if it exists, and only if not update the creation time
            int rc = lfs_file_open(&_lfs, fd.get(), path, LFS_O_RDONLY);
            if (rc == 0)
            {
                lfs_file_close(&_lfs, fd.get()); // It exists, don't update create time
            }
            else
            {
                creation = _timeCallback(); // File didn't exist or otherwise, so we're going to create this time
            }
        }

        int rc = lfs_file_open(&_lfs, fd.get(), path, flags);
        if (rc == LFS_ERR_ISDIR)
        {
            // To support the SD.openNextFile, a null FD indicates to the LittleFSFile this is just
            // a directory whose name we are carrying around but which cannot be read or written
            return std::make_shared<ext_LittleFSFileImpl>(this, path, nullptr, flags, creation);
        }
        else if (rc == 0)
        {
            lfs_file_sync(&_lfs, fd.get());
            return std::make_shared<ext_LittleFSFileImpl>(this, path, fd, flags, creation);
        }
        else
        {
            DEBUGV("LittleFSDirImpl::openFile: rc=%d fd=%p path=`%s` openMode=%d accessMode=%d err=%d\n",
                   rc, fd.get(), path, openMode, accessMode, rc);
            return FileImplPtr();
        }
    }

    /**
     * @brief OPens the directory at the specified path
     *
     * @param path to be opened
     * @return the directory implementation else nullptr (Which is the empty shared pointer)
     */
    DirImplPtr ext_LittleFSImpl::openDir(const char *path)
    {
        if (!_mounted || !path)
        {
            return DirImplPtr();
        }
        char *pathStr = strdup(path); // Allow edits on our scratch copy
        // Get rid of any trailing slashes
        while (strlen(pathStr) && (pathStr[strlen(pathStr) - 1] == '/'))
        {
            pathStr[strlen(pathStr) - 1] = 0;
        }
        // At this point we have a name of "blah/blah/blah" or "blah" or ""
        // If that references a directory, just open it and we're done.
        lfs_info info;
        auto dir = std::make_shared<lfs_dir_t>();
        int rc;
        const char *filter = "";
        if (!pathStr[0])
        {
            // openDir("") === openDir("/")
            rc = lfs_dir_open(&_lfs, dir.get(), "/");
            filter = "";
        }
        else if (lfs_stat(&_lfs, pathStr, &info) >= 0)
        {
            if (info.type == LFS_TYPE_DIR)
            {
                // Easy peasy, path specifies an existing dir!
                rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
                filter = "";
            }
            else
            {
                // This is a file, so open the containing dir
                char *ptr = strrchr(pathStr, '/');
                if (!ptr)
                {
                    // No slashes, open the root dir
                    rc = lfs_dir_open(&_lfs, dir.get(), "/");
                    filter = pathStr;
                }
                else
                {
                    // We've got slashes, open the dir one up
                    *ptr = 0; // Remove slash, truncate string
                    rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
                    filter = ptr + 1;
                }
            }
        }
        else
        {
            // Name doesn't exist, so use the parent dir of whatever was sent in
            // This is a file, so open the containing dir
            char *ptr = strrchr(pathStr, '/');
            if (!ptr)
            {
                // No slashes, open the root dir
                rc = lfs_dir_open(&_lfs, dir.get(), "/");
                filter = pathStr;
            }
            else
            {
                // We've got slashes, open the dir one up
                *ptr = 0; // Remove slash, truncate string
                rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
                filter = ptr + 1;
            }
        }
        if (rc < 0)
        {
            DEBUGV("ext_LittleFSImpl::openDir: path=`%s` err=%d\n", path, rc);
            free(pathStr);
            return DirImplPtr();
        }
        // Skip the . and .. entries
        lfs_info dirent;
        lfs_dir_read(&_lfs, dir.get(), &dirent);
        lfs_dir_read(&_lfs, dir.get(), &dirent);

        auto ret = std::make_shared<ext_LittleFSDirImpl>(filter, this, dir, pathStr);
        free(pathStr);
        return ret;
    }

    /**
     * @brief THis is the lfs flash read function, it reads the data from the internal flash!
     *
     * @param c, the configuration
     * @param block, size of the block used in the flash
     * @param off, the offset, where to start reading
     * @param dst, the destination buffer
     * @param size and the size of the data to read
     * @return int, 0 if successful (We will always return 0)
     */
    int ext_LittleFSImpl::lfs_flash_read(const struct lfs_config *c,
                                         lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size)
    {
        ext_LittleFSImpl *me = reinterpret_cast<ext_LittleFSImpl *>(c->context);
        //    Serial.printf(" READ: %p, %d\n", me->_start + (block * me->_blockSize) + off, size);
        memcpy(dst, me->_start + (block * me->_blockSize) + off, size);
        return 0;
    }

    /**
     * @brief This is the lfs flash program function, it writes the data to the internal flash
     *
     * @param c, the configuration
     * @param block, size of the block used in the flash
     * @param off, the offset, where to start writing
     * @param buffer, the source buffer
     * @param size, the size of the data to write
     * @return int, 0 if successful (We will always return 0)!
     */
    int ext_LittleFSImpl::lfs_flash_prog(const struct lfs_config *c,
                                         lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
    {
        ext_LittleFSImpl *me = reinterpret_cast<ext_LittleFSImpl *>(c->context);
        uint8_t *addr = me->_start + (block * me->_blockSize) + off;
        #if defined(USING_FREERTOS)
                // If FreeRTOS is being used, skip disabling interrupts
        #else
                noInterrupts();
        #endif
                rp2040.idleOtherCore();
                //    Serial.printf("WRITE: %p, $d\n", (intptr_t)addr - (intptr_t)XIP_BASE, size);
                flash_range_program((intptr_t)addr - (intptr_t)XIP_BASE, (const uint8_t *)buffer, size);
                rp2040.resumeOtherCore();
        #if defined(USING_FREERTOS)
                // If FreeRTOS is being used, skip enabling interrupts
        #else
                interrupts();
        #endif
                return 0;
    }

    /**
     * @brief This is the lfs flash erase function, it erases the data from the internal flash
     *
     * @param c, the configuration
     * @param block of the flash to erase
     * @return int, 0 if successful (We will always return 0)
     */
    int ext_LittleFSImpl::lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
    {
        ext_LittleFSImpl *me = reinterpret_cast<ext_LittleFSImpl *>(c->context);
        uint8_t *addr = me->_start + (block * me->_blockSize);
        //    Serial.printf("ERASE: %p, %d\n", (intptr_t)addr - (intptr_t)XIP_BASE, me->_blockSize);
        #if defined(USING_FREERTOS)
            // If FreeRTOS is being used, skip disabling interrupts
        #else
            noInterrupts();
        #endif
        rp2040.idleOtherCore();
        flash_range_erase((intptr_t)addr - (intptr_t)XIP_BASE, me->_blockSize);
        rp2040.resumeOtherCore();
        #if defined(USING_FREERTOS)
            // If FreeRTOS is being used, skip enabling interrupts
        #else
            interrupts();
        #endif
        return 0;
    }

    /**
     * @brief This is the lfs flash sync function, it syncs the data to the internal flash.
     *        But we not use the sync function, therefor this is a NOOP (No Operation)
     *
     * @param c, the configuration
     * @return int, 0 if successful (We will always return 0)
     */
    int ext_LittleFSImpl::lfs_flash_sync(const struct lfs_config *c)
    {
        /* NOOP */
        (void)c;
        return 0;
    }

}; // namespace ext_littlefs_impl

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_EXT_LITTLEFS)
uint8_t ext_FS_start = 0x000;             // Start address of the W25q128 flash memory
uint32_t ext_FS_end = FLASH_SIZE_W25Q128; // End address of the W25q128 flash memory

// Create the external LittleFS instance with the start and end address of the flash memory
FS ext_LittleFS = FS(FSImplPtr(new ext_littlefs_impl::ext_LittleFSImpl(&ext_FS_start, ext_FS_end, PAGE_SIZE_W25Q128_256B, SECTOR_SIZE_W25Q128_4KB, 16)));
#endif

#endif // ARDUINO_ARCH_RP2040
#endif // EXTERNAL_FLASH_MODULE