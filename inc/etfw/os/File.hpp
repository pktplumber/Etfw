
#pragma once

#include <cstdint>
#include <iostream>

#ifndef OS_FILE_CHUNK_SZ
#define OS_FILE_CHUNK_SZ    512
#endif

#ifndef OS_FILE_HANDLE_MAX_SZ
#define OS_FILE_HANDLE_MAX_SZ   16
#endif

#ifndef OS_FILE_HANDLE_ALIGNMENT
#define OS_FILE_HANDLE_ALIGNMENT    8
#endif

namespace Os
{
    class File
    {
        public:
            enum Mode {
                OPEN_NO_MODE,   //!<  File mode not yet selected
                OPEN_READ, //!<  Open file for reading
                OPEN_CREATE, //!< Open file for writing and truncates file if it exists, ie same flags as creat()
                OPEN_WRITE, //!<  Open file for writing
                OPEN_SYNC_WRITE, //!<  Open file for writing; writes don't return until data is on disk
                OPEN_APPEND, //!< Open file for appending
                MAX_OPEN_MODE //!< Maximum value of mode
            };

            enum Status {
                OP_OK           = 0, //!<  Operation was successful
                DOESNT_EXIST    = -1, //!<  File doesn't exist (for read)
                NO_SPACE        = -2, //!<  No space left
                NO_PERMISSION   = -3, //!<  No permission to read/write file
                BAD_SIZE        = -4, //!<  Invalid size parameter
                NOT_OPENED      = -5, //!<  file hasn't been opened yet
                FILE_EXISTS     = -6, //!< file already exist (for CREATE with O_EXCL enabled)
                NOT_SUPPORTED   = -7, //!< Kernel or file system does not support operation
                INVALID_MODE    = -8, //!< Mode for file access is invalid for current operation
                INVALID_ARGUMENT = -9, //!< Invalid argument passed in
                OTHER_ERROR      = -10, //!<  A catch-all for other errors. Have to look in implementation-specific code
                MAX_STATUS //!< Maximum value of status
            };

            enum OverwriteType {
                NO_OVERWRITE, //!< Do NOT overwrite existing files
                OVERWRITE, //!< Overwrite file when it exists and creation was requested
                MAX_OVERWRITE_TYPE
            };

            enum SeekType {
                RELATIVE, //!< Relative seek from current file offset
                ABSOLUTE, //!< Absolute seek from beginning of file
                MAX_SEEK_TYPE
            };

            enum WaitType {
                NO_WAIT, //!< Do not wait for read/write operation to finish
                WAIT, //!< Do wait for read/write operation to finish
                MAX_WAIT_TYPE
            };
            File();

            ~File() {}

            Status open(const char* path, Mode mode, OverwriteType overwrite);

            void close(void);

            Status read(uint8_t* buf, size_t &sz, WaitType wait);

            Status read(uint8_t* buf, size_t &sz) { return read(buf, sz, NO_WAIT); }

            Status readline(uint8_t* buf, size_t &sz, WaitType wait);

            Status readline(uint8_t* buf, size_t &sz) { return readline(buf, sz, NO_WAIT); }

            Status write(uint8_t* buf, size_t &sz, WaitType wait);

            Status write(uint8_t* buf, size_t &sz) { return write(buf, sz, NO_WAIT); }

            Status size(size_t &result);

            Status position(size_t &result);

            Status calc_crc(uint32_t &crc);

        private:
            int _fd;
            static const uint32_t INIT_CRC = 0xFFFFFFFF;
            Mode _mode = OPEN_NO_MODE;
            uint8_t crc_buf[OS_FILE_CHUNK_SZ];

            alignas(OS_FILE_HANDLE_ALIGNMENT) uint8_t handle[OS_FILE_HANDLE_MAX_SZ];

            Status err_to_status(int err);
    };
}
