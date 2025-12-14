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
#pragma once
#include "LittleFS.h"
#include "W25Q128.h"

using namespace fs;

namespace ext_littlefs_impl // LittleFS implementation for external flash
{

    class ext_LittleFSFileImpl; // Forward declaration of file implementation
    class ext_LittleFSDirImpl;  // Forward declaration of directory implementation

    class ext_LittleFSConfig : public FSConfig // Configuration for external flash. Using LittleFSConfig Filesystem configuration as base
    {
      public:
        static constexpr uint32_t FSId = 0x4c495454;                               // "LITT" is the LittleFS magic number
        ext_LittleFSConfig(bool autoFormat = true) : FSConfig(FSId, autoFormat) {} // Constructor with autoformat option
    };

    class ext_LittleFSImpl : public FSImpl // LittleFS implementation for external flash. Using LittleFSImpl Filesystem implementation as base
    {
      private:
        // Just a few LittleFS callbacks to be used with external flash
        using LfsReadCallback = int (*)(const struct lfs_config *, lfs_block_t, lfs_off_t, void *, lfs_size_t);
        using LfsProgCallback = int (*)(const struct lfs_config *, lfs_block_t, lfs_off_t, const void *, lfs_size_t);
        using LfsEraseCallback = int (*)(const struct lfs_config *, lfs_block_t);
        using LfsSyncCallback = int (*)(const struct lfs_config *);

      public:
        W25Q128 *extFlash = new W25Q128();

        /**
         * @brief Construct a new ext_LittleFSImpl object with full configuration support
         *
         * @param start        Set the start address of the filesystem in flash
         * @param size         Set the size of the filesystem in flash
         * @param pageSize     Set the size of a page in flash
         * @param blockSize    Set the size of a block in flash
         * @param maxOpenFds   Set the maximum number of open file descriptors
         * @param read         Pointer to the read function (optional, can be nullptr)
         * @param prog         Pointer to the program function (optional, can be nullptr)
         * @param erase        Pointer to the erase function (optional, can be nullptr)
         * @param sync         Pointer to the sync function (optional, can be nullptr)
         */
        ext_LittleFSImpl(uint8_t *start,                   // Start address of the filesystem in flash
                         uint32_t size,                    // Size of the filesystem in flash
                         uint32_t pageSize,                // Size of a page in flash
                         uint32_t blockSize,               // Size of a block in flash
                         uint32_t maxOpenFds,              // Maximum number of open file descriptors
                         LfsReadCallback read = nullptr,   // Callback for reading data
                         LfsProgCallback prog = nullptr,   // Callback for writing data
                         LfsEraseCallback erase = nullptr, // Callback for erasing data
                         LfsSyncCallback sync = nullptr)   // Callback for syncing data
            : _start(start),                               // Start address of the filesystem in flash
              _size(size),                                 // Size of the filesystem in flash
              _pageSize(pageSize),                         // Size of a page in flash
              _blockSize(blockSize),                       // Size of a block in flash
              _maxOpenFds(maxOpenFds),                     // Maximum number of open file descriptors
              _mounted(false)                              // Whether the filesystem is mounted
        {
            memset(&_lfs, 0, sizeof(_lfs));         // Clear the LittleFS context
            memset(&_lfs_cfg, 0, sizeof(_lfs_cfg)); // Clear the LittleFS configuration

            // Context
            _lfs_cfg.context = (void *)this; // Context for LittleFS

            // Function callbacks
            _lfs_cfg.read = read ? read : lfs_flash_read;     // read function. Fall back to lfs_flash_read if nullptr
            _lfs_cfg.prog = prog ? prog : lfs_flash_prog;     // Program function. Fall back to lfs_flash_prog if nullptr
            _lfs_cfg.erase = erase ? erase : lfs_flash_erase; // Erase function. Fall back to lfs_flash_erase if nullptr
            _lfs_cfg.sync = sync ? sync : lfs_flash_sync;     // Sync function. Fall back to lfs_flash_sync if nullptr

#ifdef LFS_THREADSAFE
            // Thread safety (if required)
            _lfs_cfg.lock = nullptr;   // Lock function
            _lfs_cfg.unlock = nullptr; // Unlock function
#endif
            // Size configurations
            _lfs_cfg.read_size = pageSize;                              // Minimal read size
            _lfs_cfg.prog_size = pageSize;                              // Minimal program size
            _lfs_cfg.block_size = _blockSize;                           // Block size in flash
            _lfs_cfg.block_count = _blockSize ? _size / _blockSize : 0; // Number of blocks in flash
            _lfs_cfg.block_cycles = 500;                                // Number of write cycles per block
            _lfs_cfg.cache_size = pageSize;                             // Cache size
            _lfs_cfg.lookahead_size = 16;                               // Lookahead buffer size
            _lfs_cfg.compact_thresh = 0;                                // Compact threshold (default)

            // Static buffers for LittleFS (optional, set to nullptr)
            _lfs_cfg.read_buffer = nullptr;      // Read buffer
            _lfs_cfg.prog_buffer = nullptr;      // Program buffer
            _lfs_cfg.lookahead_buffer = nullptr; // Lookahead buffer

            // Maximum sizes
            _lfs_cfg.name_max = 255;   // Maximum filename length
            _lfs_cfg.file_max = 0;     // Maximum number of files open at the same time. 0 for default setting
            _lfs_cfg.attr_max = 0;     // Maximum number of attributes (0 for default)
            _lfs_cfg.metadata_max = 0; // Maximum metadata size (0 for default)
            _lfs_cfg.inline_max = 0;   // Maximum inline data size (0 for default)

#ifdef LFS_MULTIVERSION
            // Disk version (if multiversion support is enabled)
            _lfs_cfg.disk_version = 0; // Default disk version
#endif
        }

        /**
         * @brief Destroy the ext LittleFSImpl object
         */
        ~ext_LittleFSImpl()
        {
            if (_mounted) lfs_unmount(&_lfs); // Unmount the filesystem if it is mounted
        }

        FileImplPtr open(const char *path, OpenMode openMode, AccessMode accessMode) override; // Open a file, return a file implementation
        DirImplPtr openDir(const char *path) override;                                         // Open a directory, return a directory implementation

        bool setLFSConfig(const lfs_config &cfg) // Set the LittleFS configuration
        {
            _lfs_cfg = cfg;                                          // Set the LittleFS configuration
            return memcmp(&_lfs_cfg, &cfg, sizeof(lfs_config)) == 0; // Verify the values were copied correctly
        }

        const lfs_config &getLFSConfig() const { return _lfs_cfg; } // Get the current LittleFS configuration

        // Setters for the internal LittleFS configuration
        void setReadFunction(LfsReadCallback read) { _lfs_cfg.read = read; }      // Set the read function
        void setProgFunction(LfsProgCallback prog) { _lfs_cfg.prog = prog; }      // Set the program function
        void setEraseFunction(LfsEraseCallback erase) { _lfs_cfg.erase = erase; } // Set the erase function
        void setSyncFunction(LfsSyncCallback sync) { _lfs_cfg.sync = sync; }      // Set the sync function
#ifdef LFS_THREADSAFE
        void setLockFunction(LfsLockCallback lock) { _lfs_cfg.lock = lock; }           // Set the lock function
        void setUnlockFunction(LfsUnlockCallback unlock) { _lfs_cfg.unlock = unlock; } // Set the unlock function
#endif
        void setReadSize(uint32_t readSize) { _lfs_cfg.read_size = readSize; }                          // Set the read size
        void setProgSize(uint32_t progSize) { _lfs_cfg.prog_size = progSize; }                          // Set the program size
        void setBlockSize(uint32_t blockSize) { _lfs_cfg.block_size = blockSize; }                      // Set the block size
        void setBlockCount(uint32_t blockCount) { _lfs_cfg.block_count = blockCount; }                  // Set the block count
        void setBlockCycles(uint32_t blockCycles) { _lfs_cfg.block_cycles = blockCycles; }              // Set the block cycles
        void setCacheSize(uint32_t cacheSize) { _lfs_cfg.cache_size = cacheSize; }                      // Set the cache size
        void setLookaheadSize(uint32_t lookaheadSize) { _lfs_cfg.lookahead_size = lookaheadSize; }      // Set the lookahead size
        void setCompactThresh(uint32_t compactThresh) { _lfs_cfg.compact_thresh = compactThresh; }      // Set the compact threshold
        void setReadBuffer(void *readBuffer) { _lfs_cfg.read_buffer = readBuffer; }                     // Set the read buffer
        void setProgBuffer(void *progBuffer) { _lfs_cfg.prog_buffer = progBuffer; }                     // Set the program buffer
        void setLookaheadBuffer(void *lookaheadBuffer) { _lfs_cfg.lookahead_buffer = lookaheadBuffer; } // Set the lookahead buffer
        void setNameMax(uint32_t nameMax) { _lfs_cfg.name_max = nameMax; }                              // Set the maximum filename length
        void setFileMax(uint32_t fileMax) { _lfs_cfg.file_max = fileMax; }                              // Set the maximum number of files open at the same time
        void setAttrMax(uint32_t attrMax) { _lfs_cfg.attr_max = attrMax; }                              // Set the maximum number of attributes
        void setMetadataMax(uint32_t metadataMax) { _lfs_cfg.metadata_max = metadataMax; }              // Set the maximum metadata size
        void setInlineMax(uint32_t inlineMax) { _lfs_cfg.inline_max = inlineMax; }                      // Set the maximum inline data size
#ifdef LFS_MULTIVERSION
        void setDiskVersion(uint32_t diskVersion) { _lfs_cfg.disk_version = diskVersion; } // Set the disk version
#endif

        // Getters for the internal LittleFS configuration
        LfsReadCallback getReadFunction() const { return _lfs_cfg.read; }    // Get the read function
        LfsProgCallback getProgFunction() const { return _lfs_cfg.prog; }    // Get the program function
        LfsEraseCallback getEraseFunction() const { return _lfs_cfg.erase; } // Get the erase function
        LfsSyncCallback getSyncFunction() const { return _lfs_cfg.sync; }    // Get the sync function
#ifdef LFS_THREADSAFE
        LfsLockCallback getLockFunction() const { return _lfs_cfg.lock; }       // Get the lock function
        LfsUnlockCallback getUnlockFunction() const { return _lfs_cfg.unlock; } // Get the unlock function
#endif
        uint32_t getReadSize() const { return _lfs_cfg.read_size; }            // Get the read size
        uint32_t getProgSize() const { return _lfs_cfg.prog_size; }            // Get the program size
        uint32_t getBlockSize() const { return _lfs_cfg.block_size; }          // Get the block size
        uint32_t getBlockCount() const { return _lfs_cfg.block_count; }        // Get the block count
        uint32_t getBlockCycles() const { return _lfs_cfg.block_cycles; }      // Get the block cycles
        uint32_t getCacheSize() const { return _lfs_cfg.cache_size; }          // Get the cache size
        uint32_t getLookaheadSize() const { return _lfs_cfg.lookahead_size; }  // Get the lookahead size
        uint32_t getCompactThresh() const { return _lfs_cfg.compact_thresh; }  // Get the compact threshold
        void *getReadBuffer() const { return _lfs_cfg.read_buffer; }           // Get the read buffer
        void *getProgBuffer() const { return _lfs_cfg.prog_buffer; }           // Get the program buffer
        void *getLookaheadBuffer() const { return _lfs_cfg.lookahead_buffer; } // Get the lookahead buffer
        uint32_t getNameMax() const { return _lfs_cfg.name_max; }              // Get the maximum filename length
        uint32_t getFileMax() const { return _lfs_cfg.file_max; }              // Get the maximum number of files open at the same time
        uint32_t getAttrMax() const { return _lfs_cfg.attr_max; }              // Get the maximum number of attributes
        uint32_t getMetadataMax() const { return _lfs_cfg.metadata_max; }      // Get the maximum metadata size
        uint32_t getInlineMax() const { return _lfs_cfg.inline_max; }          // Get the maximum inline data size
#ifdef LFS_MULTIVERSION
        uint32_t getDiskVersion() const { return _lfs_cfg.disk_version; } // Get the disk version
#endif

        /**
         * @brief Check if a file or directory exists
         *
         * @param path to the file or directory
         * @return true if the file or directory exists else false
         *
         */
        bool exists(const char *path) override
        {
            if (!_mounted || !path || !path[0])
            {
                return false;
            }
            lfs_info info;
            const int rc = lfs_stat(&_lfs, path, &info);
            return (rc == 0);
        }

        /**
         * @brief Rename a file or directory
         *
         * @param pathFrom the original path
         * @param pathTo the new path
         * @return true if the file or directory was renamed else false
         *
         */
        bool rename(const char *pathFrom, const char *pathTo) override
        {
            if (!_mounted || !pathFrom || !pathFrom[0] || !pathTo || !pathTo[0])
            {
                return false;
            }
            const int rc = lfs_rename(&_lfs, pathFrom, pathTo);
            if (rc != 0)
            {
                DEBUGV("lfs_rename: rc=%d, from=`%s`, to=`%s`\n", rc, pathFrom, pathTo);
                return false;
            }
            return true;
        }

        /**
         * @brief Set the requested filesystem configuration
         *
         * @param info the filesystem configuration
         * @return true if the device is mounted else false
         */
        bool info(FSInfo &info) override
        {
            if (!_mounted)
            {
                return false;
            }
            info.blockSize = _blockSize;
            info.pageSize = _pageSize;
            info.maxOpenFiles = _maxOpenFds;
            info.maxPathLength = LFS_NAME_MAX;
            info.totalBytes = _size;
            info.usedBytes = _getUsedBlocks() * _blockSize;
            return true;
        }

        /**
         * @brief Remo the file or directory including the subdirectories
         *
         * @param path to the file or directory
         * @return true if the file or directory was removed else false
         */
        bool remove(const char *path) override
        {
            if (!_mounted || !path || !path[0])
            {
                return false;
            }
            int rc = lfs_remove(&_lfs, path);
            if (rc != 0)
            {
                DEBUGV("lfs_remove: rc=%d path=`%s`\n", rc, path);
                return false;
            }
            // Now try and remove any empty subdirs this makes, silently
            char *pathStr = strdup(path);
            if (pathStr)
            {
                char *ptr = strrchr(pathStr, '/');
                while (ptr)
                {
                    *ptr = 0;
                    lfs_remove(&_lfs, pathStr); // Don't care if fails if there are files left
                    ptr = strrchr(pathStr, '/');
                }
                free(pathStr);
            }
            return true;
        }

        /**
         * @brief Create a directory and set the creation time as a attribute
         *
         * @param path to the directory that should be created
         * @return true if the directory was created else false
         */
        bool mkdir(const char *path) override
        {
            if (!_mounted || !path || !path[0])
            {
                return false;
            }
            int rc = lfs_mkdir(&_lfs, path);
            if ((rc == 0) && _timeCallback)
            {
                time_t now = _timeCallback();
                // Add metadata with creation time to the directory marker
                int rc = lfs_setattr(&_lfs, path, 'c', (const void *)&now, sizeof(now));
                if (rc < 0)
                {
                    DEBUGV("Unable to set creation time on '%s' to %ld\n", path, (long)now);
                }
            }
            return (rc == 0);
        }

        /**
         * @brief Remove a directory and all its content
         *
         * @param path to the directory that should be removed
         * @return true if the directory was removed else false
         */
        bool rmdir(const char *path) override
        {
            return remove(path); // Same call on LittleFS
        }

        /**
         * @brief Set the requested filesystem configuration
         *
         * @param cfg the filesystem configuration
         * @return true if the configuration was set else false
         */
        bool setConfig(const FSConfig &cfg) override
        {
            if ((cfg._type != ext_LittleFSConfig::FSId) || _mounted)
            {
                return false;
            }
            _cfg = *static_cast<const ext_LittleFSConfig *>(&cfg);
            return true;
        }

        /**
         * @brief Initialize the filesystem. If the filesystem is not mounted
         *        and autoformat is disabled, the filesystem will not be formatted
         *
         * @return true if the filesystem was initialized else false
         */
        bool begin() override
        {
            if (extFlash->begin())
            {
                if (_mounted)
                {
                    return true;
                }
                if (_size <= 0)
                {
                    DEBUGV("LittleFS size is <= zero");
                    return false;
                }
                if (_tryMount())
                {
                    return true;
                }
                if (!_cfg._autoFormat || !format())
                {
                    return false;
                }
                return _tryMount();
            }
            else
            {
                DEBUGV("ext flash not initialized");
                return false;
            }
        }

        /**
         * @brief Unmount the filesystem
         */
        void end() override
        {
            if (!_mounted)
            {
                return;
            }
            lfs_unmount(&_lfs);
            _mounted = false;
        }

        /**
         * @brief Format the filesystem including the creation time attribute
         *
         * @return true if the filesystem was formatted else false
         */
        bool format() override
        {
            if (_size == 0)
            {
                DEBUGV("lfs size is zero\n");
                return false;
            }

            bool wasMounted = _mounted;
            if (_mounted)
            {
                lfs_unmount(&_lfs);
                _mounted = false;
            }

            memset(&_lfs, 0, sizeof(_lfs));
            int rc = lfs_format(&_lfs, &_lfs_cfg);
            if (rc != 0)
            {
                DEBUGV("lfs_format: rc=%d\n", rc);
                return false;
            }

            if (_timeCallback && _tryMount())
            {
                // Mounting is required to set attributes

                time_t t = _timeCallback();
                rc = lfs_setattr(&_lfs, "/", 'c', &t, 8);
                if (rc != 0)
                {
                    DEBUGV("lfs_format, lfs_setattr 'c': rc=%d\n", rc);
                    return false;
                }

                rc = lfs_setattr(&_lfs, "/", 't', &t, 8);
                if (rc != 0)
                {
                    DEBUGV("lfs_format, lfs_setattr 't': rc=%d\n", rc);
                    return false;
                }

                lfs_unmount(&_lfs);
                _mounted = false;
            }

            if (wasMounted)
            {
                return _tryMount();
            }

            return true;
        }

        /**
         * @brief Collects the status information of a file or directory
         *
         * @param path to
         * @param st the filesystem stat structure
         * @return true if the file or directory was found else false
         */
        bool stat(const char *path, FSStat *st) override
        {
            if (!_mounted || !path || !path[0])
            {
                return false;
            }
            lfs_info info;
            if (lfs_stat(&_lfs, path, &info) < 0)
            {
                return false;
            }
            st->size = info.size;
            st->blocksize = _blockSize;
            st->isDir = info.type == LFS_TYPE_DIR;
            if (st->isDir)
            {
                st->size = 0;
            }
            if (lfs_getattr(&_lfs, path, 'c', (void *)&st->ctime, sizeof(st->ctime)) != sizeof(st->ctime))
            {
                st->ctime = 0;
            }
            st->atime = st->ctime;
            return true;
        }

        /**
         * @brief Get the creation time of a file or directory
         *
         * @return the creation time of the file or directory
         */
        time_t getCreationTime() override
        {
            time_t t;
            uint32_t t32b;

            if (lfs_getattr(&_lfs, "/", 'c', &t, 8) == 8)
            {
                return t;
            }
            else if (lfs_getattr(&_lfs, "/", 'c', &t32b, 4) == 4)
            {
                return (time_t)t32b;
            }
            else
            {
                return 0;
            }
        }

      protected:
        friend class ext_LittleFSFileImpl; // Our super fiendliest class File System File Implementation
        friend class ext_LittleFSDirImpl;  // Our super fiendliest class File System Directory Implementation

        /**
         * @brief Gets the file system context
         *
         * @return lfs_t* the file system context
         */
        lfs_t *getFS()
        {
            return &_lfs;
        }

        /**
         * @brief Try to mount the filesystem
         *
         * @return true if the filesystem was mounted else false
         */
        bool _tryMount()
        {
            if (_mounted)
            {
                lfs_unmount(&_lfs);
                _mounted = false;
            }
            memset(&_lfs, 0, sizeof(_lfs));
            const int rc = lfs_mount(&_lfs, &_lfs_cfg);
            if (rc == 0)
            {
                _mounted = true;
            }
            return _mounted;
        }

        /**
         * @brief Get the number of used blocks
         *
         * @return the number of used blocks
         */
        int _getUsedBlocks()
        {
            if (!_mounted)
            {
                return 0;
            }
            return lfs_fs_size(&_lfs);
        }

        /**
         * @brief Get the flags for the open mode and access mode
         *
         * @param openMode the open mode
         * @param accessMode the access mode
         * @return the flags
         */
        static int _getFlags(OpenMode openMode, AccessMode accessMode)
        {
            int mode = 0;
            if (openMode & OM_CREATE) // Create file if it does not exist
            {
                mode |= LFS_O_CREAT;
            }

            if (openMode & OM_APPEND) // Append data to the end of the file
            {
                mode |= LFS_O_APPEND;
            }

            if (openMode & OM_TRUNCATE) // Truncate the file to zero length
            {
                mode |= LFS_O_TRUNC;
            }

            if (accessMode & AM_READ) // Open the file for reading
            {
                mode |= LFS_O_RDONLY;
            }

            if (accessMode & AM_WRITE) // Open the file for writing
            {
                mode |= LFS_O_WRONLY;
            }
            return mode;
        }

        /**
         * @brief Check if the path is valid
         *
         * @param path the path to check
         * @return true if the path is valid else false
         */
        static bool pathValid(const char *path)
        {
            while (*path)
            {
                const char *slash = strchr(path, '/');
                if (!slash)
                {
                    if (strlen(path) >= LFS_NAME_MAX)
                    {
                        // Terminal filename is too long
                        return false;
                    }
                    break;
                }
                if ((slash - path) >= LFS_NAME_MAX)
                {
                    // This subdir name too long
                    return false;
                }
                path = slash + 1;
            }
            return true;
        }

        // The actual flash accessing routines
        static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t off, void *buffer, lfs_size_t size);
        static int lfs_flash_prog(const struct lfs_config *c, lfs_block_t block,
                                  lfs_off_t off, const void *buffer, lfs_size_t size);
        static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block);
        static int lfs_flash_sync(const struct lfs_config *c);

        lfs_t _lfs;          // LittleFS context, the filesystem
        lfs_config _lfs_cfg; // LittleFS configuration, the filesystem configuration

        ext_LittleFSConfig _cfg; // Our configuration

        uint8_t *_start;      // Start address of the filesystem in flash
        uint32_t _size;       // Size of the filesystem in flash
        uint32_t _pageSize;   // Size of a page in flash
        uint32_t _blockSize;  // Size of a block in flash
        uint32_t _maxOpenFds; // Maximum number of open file descriptors

        bool _mounted; // Whether the filesystem is mounted
    };

    class ext_LittleFSFileImpl : public FileImpl
    {
      public:
        /**
         * @brief Construct a new ext_LittleFSFileImpl object
         *
         * @param fs the filesystem implementation
         * @param name the name of the file
         * @param fd the file descriptor
         * @param flags the flags
         * @param creation the creation time
         */
        ext_LittleFSFileImpl(ext_LittleFSImpl *fs, const char *name, std::shared_ptr<lfs_file_t> fd, int flags, time_t creation) : _fs(fs), _fd(fd), _opened(true), _flags(flags), _creation(creation)
        {
            _name = std::shared_ptr<char>(new char[strlen(name) + 1], std::default_delete<char[]>());
            strcpy(_name.get(), name);
        }

        /**
         * @brief Destroy the ext LittleFSFileImpl object
         */
        ~ext_LittleFSFileImpl() override
        {
            if (_opened)
            {
                close();
            }
        }

        /**
         * @brief Write data to the file
         *
         * @param buf the buffer with the data
         * @param size the size of the data
         * @return the number of bytes written
         */
        size_t write(const uint8_t *buf, size_t size) override
        {
            if (!_opened || !_fd || !buf)
            {
                return 0;
            }
            int result = lfs_file_write(_fs->getFS(), _getFD(), (void *)buf, size);
            if (result < 0)
            {
                DEBUGV("lfs_write rc=%d\n", result);
                return 0;
            }
            return result;
        }

        /**
         * @brief Read data from the file
         *
         * @param buf the buffer to read the data into
         * @param size the size of the buffer
         * @return int the number of bytes read
         */
        int read(uint8_t *buf, size_t size) override
        {
            if (!_opened || !_fd | !buf)
            {
                return 0;
            }
            int result = lfs_file_read(_fs->getFS(), _getFD(), (void *)buf, size);
            if (result < 0)
            {
                DEBUGV("lfs_read rc=%d\n", result);
                return 0;
            }

            return result;
        }

        /**
         * @brief flush the file, which means writing all data to the file
         *
         */
        void flush() override
        {
            if (!_opened || !_fd)
            {
                return;
            }
            int rc = lfs_file_sync(_fs->getFS(), _getFD());
            if (rc < 0)
            {
                DEBUGV("lfs_file_sync rc=%d\n", rc);
            }
        }

        /**
         * @brief seek to a specific position in the file
         *
         * @param pos the position to seek to
         * @param mode the seek mode
         * @return true  if the seek was successful, otherwise false
         */
        bool seek(uint32_t pos, SeekMode mode) override
        {
            if (!_opened || !_fd)
            {
                return false;
            }
            int32_t offset = static_cast<int32_t>(pos);
            if (mode == SeekEnd)
            {
                offset = -offset; // TODO - this seems like its plain wrong vs. POSIX
            }
            auto lastPos = position();
            int rc = lfs_file_seek(_fs->getFS(), _getFD(), offset, (int)mode); // NB. SeekMode === LFS_SEEK_TYPES
            if (rc < 0)
            {
                DEBUGV("lfs_file_seek rc=%d\n", rc);
                return false;
            }
            if (position() > size())
            {
                seek(lastPos, SeekSet); // Pretend the seek() never happened
                return false;
            }
            return true;
        }

        /**
         * @brief get the current position in the file
         *
         * @return size_t of the current position
         */
        size_t position() const override
        {
            if (!_opened || !_fd)
            {
                return 0;
            }
            int result = lfs_file_tell(_fs->getFS(), _getFD());
            if (result < 0)
            {
                DEBUGV("lfs_file_tell rc=%d\n", result);
                return 0;
            }
            return result;
        }

        /**
         * @brief Get the size of the file
         *
         * @return size_t the size of the file
         */
        size_t size() const override
        {
            return (_opened && _fd) ? lfs_file_size(_fs->getFS(), _getFD()) : 0;
        }

        /**
         * @brief truncate the file to a specific size, which means the file will be cut off at the specified size
         *
         * @param size the size to truncate the file to
         * @return true if the file was truncated, otherwise false
         */
        bool truncate(uint32_t size) override
        {
            if (!_opened || !_fd)
            {
                return false;
            }
            int rc = lfs_file_truncate(_fs->getFS(), _getFD(), size);
            if (rc < 0)
            {
                DEBUGV("lfs_file_truncate rc=%d\n", rc);
                return false;
            }
            return true;
        }

        /**
         * @brief Close the file. If the file was opened with O_CREAT, write the
         *        creation time attribute and add metadata with last write time
         */
        void close() override
        {
            if (_opened && _fd)
            {
                lfs_file_close(_fs->getFS(), _getFD());
                _opened = false;
                DEBUGV("lfs_file_close: fd=%p\n", _getFD());
                if (_timeCallback && (_flags & LFS_O_WRONLY))
                {
                    // If the file opened with O_CREAT, write the creation time attribute
                    if (_creation)
                    {
                        int rc = lfs_setattr(_fs->getFS(), _name.get(), 'c', (const void *)&_creation, sizeof(_creation));
                        if (rc < 0)
                        {
                            DEBUGV("Unable to set creation time on '%s' to %lld\n", _name.get(), _creation);
                        }
                    }
                    // Add metadata with last write time
                    time_t now = _timeCallback();
                    int rc = lfs_setattr(_fs->getFS(), _name.get(), 't', (const void *)&now, sizeof(now));
                    if (rc < 0)
                    {
                        DEBUGV("Unable to set last write time on '%s' to %lld\n", _name.get(), now);
                    }
                }
            }
        }

        /**
         * @brief Get the Last Write object
         *
         * @return time_t
         */
        time_t getLastWrite() override
        {
            time_t ftime = 0;
            if (_opened && _fd)
            {
                int rc = lfs_getattr(_fs->getFS(), _name.get(), 't', (void *)&ftime, sizeof(ftime));
                if (rc != sizeof(ftime))
                {
                    ftime = 0; // Error, so clear read value
                }
            }
            return ftime;
        }

        /**
         * @brief Get the Creation Time object
         *
         * @return time_t
         */
        time_t getCreationTime() override
        {
            time_t ftime = 0;
            if (_opened && _fd)
            {
                int rc = lfs_getattr(_fs->getFS(), _name.get(), 'c', (void *)&ftime, sizeof(ftime));
                if (rc != sizeof(ftime))
                {
                    ftime = 0; // Error, so clear read value
                }
            }
            return ftime;
        }

        /**
         * @brief Get the name of the file. Removes the path and returns only the filename
         *
         * @return const char*
         */
        const char *name() const override
        {
            if (!_opened) // File is not open. Return nullptr
            {
                return nullptr;
            }
            else
            {
                const char *p = _name.get();                // Start at the beginning
                const char *slash = strrchr(p, '/');        // Find the last slash
                return (slash && slash[1]) ? slash + 1 : p; // Return the filename
            }
        }

        /**
         * @brief Get the full name of the file. Which includes all the path
         *
         * @return const char*
         */
        const char *fullName() const override
        {
            return _opened ? _name.get() : nullptr;
        }

        /**
         * @brief Check if the file is a directory
         *
         * @return true if the file is a directory, otherwise false
         */
        bool isFile() const override
        {
            if (!_opened || !_fd)
            {
                return false;
            }
            lfs_info info;
            int rc = lfs_stat(_fs->getFS(), fullName(), &info);
            return (rc == 0) && (info.type == LFS_TYPE_REG);
        }

        /**
         * @brief Check if it is a directory
         *
         * @return true, if it is a directory, otherwise false
         */
        bool isDirectory() const override
        {
            if (!_opened)
            {
                return false;
            }
            else if (!_fd)
            {
                return true;
            }
            lfs_info info;
            int rc = lfs_stat(_fs->getFS(), fullName(), &info);
            return (rc == 0) && (info.type == LFS_TYPE_DIR);
        }

      protected:
        /**
         * @brief get the file descriptor
         *
         * @return lfs_file_t*
         */
        lfs_file_t *_getFD() const
        {
            return _fd.get();
        }

        ext_LittleFSImpl *_fs;           // The filesystem implementation
        std::shared_ptr<lfs_file_t> _fd; // The file descriptor
        std::shared_ptr<char> _name;     // The name of the file
        bool _opened;                    // Whether the file is opened
        int _flags;                      // The flags
        time_t _creation;                // The creation time
    };

    class ext_LittleFSDirImpl : public DirImpl
    {
      public:
        /**
         * @brief Construct a new ext LittleFSDirImpl object
         *
         * @param pattern, the pattern to match
         * @param fs, the filesystem implementation
         * @param dir, the directory
         * @param dirPath, the path of the directory
         */
        ext_LittleFSDirImpl(const String &pattern, ext_LittleFSImpl *fs, std::shared_ptr<lfs_dir_t> dir, const char *dirPath = nullptr)
            : _pattern(pattern), _fs(fs), _dir(dir), _dirPath(nullptr), _valid(false), _opened(true)
        {
            memset(&_dirent, 0, sizeof(_dirent));
            if (dirPath)
            {
                _dirPath = std::shared_ptr<char>(new char[strlen(dirPath) + 1], std::default_delete<char[]>());
                strcpy(_dirPath.get(), dirPath);
            }
        }

        /**
         * @brief Destroy the ext LittleFSDirImpl object.
         *        Closes if opened
         */
        ~ext_LittleFSDirImpl() override
        {
            if (_opened)
            {
                lfs_dir_close(_fs->getFS(), _getDir());
            }
        }

        /**
         * @brief Open a file
         *
         * @param openMode the open mode
         * @param accessMode the access mode
         * @return FileImplPtr the file implementation
         */
        FileImplPtr openFile(OpenMode openMode, AccessMode accessMode) override
        {
            if (!_valid)
            {
                return FileImplPtr();
            }
            int nameLen = 3; // Slashes, terminator
            nameLen += _dirPath.get() ? strlen(_dirPath.get()) : 0;
            nameLen += strlen(_dirent.name);
            char tmpName[nameLen];
            snprintf(tmpName, nameLen, "%s%s%s", _dirPath.get() ? _dirPath.get() : "", _dirPath.get() && _dirPath.get()[0] ? "/" : "", _dirent.name);
            auto ret = _fs->open((const char *)tmpName, openMode, accessMode);
            return ret;
        }

        /**
         * @brief get the name of the file
         *
         * @return const char*
         */
        const char *fileName() override
        {
            if (!_valid)
            {
                return nullptr;
            }
            return (const char *)_dirent.name;
        }

        /**
         * @brief get the size of the file
         *
         * @return size_t
         */
        size_t fileSize() override
        {
            if (!_valid)
            {
                return 0;
            }
            return _dirent.size;
        }

        /**
         * @brief Get the Open Time object, which is the time the file was opened
         *
         * @return time_t
         */
        time_t fileTime() override
        {
            time_t t;
            int32_t t32b;

            // If the attribute is 8-bytes, we're all set
            if (_getAttr('t', 8, &t))
            {
                return t;
            }
            else if (_getAttr('t', 4, &t32b))
            {
                // If it's 4 bytes silently promote to 64b
                return (time_t)t32b;
            }
            else
            {
                // OTW, none present
                return 0;
            }
        }

        /**
         * @brief Get the file Creation Time
         *
         * @return time_t
         */
        time_t fileCreationTime() override
        {
            time_t t;
            int32_t t32b;

            // If the attribute is 8-bytes, we're all set
            if (_getAttr('c', 8, &t))
            {
                return t;
            }
            else if (_getAttr('c', 4, &t32b))
            {
                // If it's 4 bytes silently promote to 64b
                return (time_t)t32b;
            }
            else
            {
                // OTW, none present
                return 0;
            }
        }

        /**
         * @brief Is it a file
         * @return true if it is
         */
        bool isFile() const override
        {
            return _valid && (_dirent.type == LFS_TYPE_REG);
        }

        /**
         * @brief Is it a directory
         * @return true if it is
         */
        bool isDirectory() const override
        {
            return _valid && (_dirent.type == LFS_TYPE_DIR);
        }

        /**
         * @brief Rewind the directory, which means go back to the beginning.
         *        This is normally used to start reading the directory from the beginning
         * @return true if successful
         */
        bool rewind() override
        {
            _valid = false;
            int rc = lfs_dir_rewind(_fs->getFS(), _getDir());
            // Skip the . and .. entries
            lfs_info dirent;
            lfs_dir_read(_fs->getFS(), _getDir(), &dirent);
            lfs_dir_read(_fs->getFS(), _getDir(), &dirent);
            return (rc == 0);
        }

        /**
         * @brief Go to the next entry in the directory
         * @return true if successful
         */
        bool next() override
        {
            const int n = _pattern.length();
            bool match;
            do
            {
                _dirent.name[0] = 0;
                int rc = lfs_dir_read(_fs->getFS(), _getDir(), &_dirent);
                _valid = (rc == 1);
                match = (!n || !strncmp((const char *)_dirent.name, _pattern.c_str(), n));
            }
            while (_valid && !match);
            return _valid;
        }

      protected:
        /**
         * @brief Get the directory, which it is
         *
         * @return lfs_dir_t*
         */
        lfs_dir_t *_getDir() const
        {
            return _dir.get();
        }

        /**
         * @brief Get the attribute of a file
         *
         * @param attr the attribute to get
         * @param len the length of the attribute
         * @param dest the destination of the attribute
         * @return true if successful
         */
        bool _getAttr(char attr, int len, void *dest)
        {
            if (!_valid || !len || !dest)
            {
                return false;
            }
            int nameLen = 3; // Slashes, terminator
            nameLen += _dirPath.get() ? strlen(_dirPath.get()) : 0;
            nameLen += strlen(_dirent.name);
            char tmpName[nameLen];
            snprintf(tmpName, nameLen, "%s%s%s", _dirPath.get() ? _dirPath.get() : "", _dirPath.get() && _dirPath.get()[0] ? "/" : "", _dirent.name);
            int rc = lfs_getattr(_fs->getFS(), tmpName, attr, dest, len);
            return (rc == len);
        }

        String _pattern;                 // THe pattern of the
        ext_LittleFSImpl *_fs;           // The filesystem implementation
        std::shared_ptr<lfs_dir_t> _dir; // The directory
        std::shared_ptr<char> _dirPath;  // The path of the directory
        lfs_info _dirent;                // The directory entry
        bool _valid;                     // Whether is valid or not
        bool _opened;                    // Whether is opened or not
    };

}; // namespace ext_littlefs_impl

#if !defined(NO_GLOBAL_INSTANCE) && !defined(NO_GLOBAL_EXT_LITTLEFS)
extern FS ext_LittleFS;
extern uint8_t ext_FS_start;
extern uint32_t ext_FS_end;
using ext_littlefs_impl::ext_LittleFSConfig;
#endif // ARDUINO
#endif 

#endif
