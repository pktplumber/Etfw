
#include "os/File.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "etfw_assert.hpp"

using namespace Os;

static constexpr int UNOPEN_FD = -1;
static constexpr int ERR_RC = -1;

File::File(): _fd(UNOPEN_FD) {}

File::Status File::err_to_status(int err)
{
    File::Status status = File::Status::OP_OK;
    switch (err) {
        case 0:
            status = File::Status::OP_OK;
            break;
        // Fallthrough intended
        case ENOSPC:
        case EFBIG:
            status = File::Status::NO_SPACE;
            break;
        case ENOENT:
            status = File::Status::DOESNT_EXIST;
            break;
        // Fallthrough intended
        case EPERM:
        case EACCES:
            status = File::Status::NO_PERMISSION;
            break;
        case EEXIST:
            status = File::Status::FILE_EXISTS;
            break;
        case EBADF:
            status = File::Status::NOT_OPENED;
            break;
        // Fallthrough intended
        case ENOSYS:
        case EOPNOTSUPP:
            status = File::Status::NOT_SUPPORTED;
            break;
        case EINVAL:
            status = File::Status::INVALID_ARGUMENT;
            break;
        default:
            status = File::Status::OTHER_ERROR;
            break;
    }
    return status;
}

File::Status File::open(const char* path, File::Mode mode, File::OverwriteType overwrite)
{
    ETFW_ASSERT(path != nullptr, "File path cannot be null");
    ETFW_ASSERT(mode > File::OPEN_NO_MODE && mode < File::MAX_OPEN_MODE,
        "Invalid mode selected");
    ETFW_ASSERT(overwrite >= File::NO_OVERWRITE && overwrite <= File::MAX_OVERWRITE_TYPE,
        "Invalid overwrite type selected");
    int32_t flags = 0;
    File::Status status = File::OP_OK;

    switch (mode)
    {
    case File::OPEN_READ:
        flags = O_RDONLY;
        break;
    
    case File::OPEN_CREATE:
        flags = O_WRONLY | O_CREAT | O_TRUNC | ((overwrite == File::OVERWRITE) ? 0 : O_EXCL);
        break;
    
    case File::OPEN_WRITE:
        flags = O_WRONLY | O_CREAT;
        break;
    
    case File::OPEN_SYNC_WRITE:
        flags = O_WRONLY | O_CREAT | O_SYNC;
        break;
    
    case File::OPEN_APPEND:
        flags = O_WRONLY | O_CREAT | O_APPEND;
        break;

    default:
        ETFW_ASSERT(0, "Unknown open mode");
        break;
    }

    _fd = ::open(path, flags, S_IRUSR | S_IWUSR);
    if (_fd == UNOPEN_FD)
    {
        int _errno = errno;
        status = err_to_status(_errno);
    }
    else
    {
        _mode = mode;
    }
    return status;
}

void File::close(void)
{
    if (_fd != UNOPEN_FD)
    {
        ::close(_fd);
        _fd = UNOPEN_FD;
        _mode = File::OPEN_NO_MODE;
    }
}

File::Status File::read(uint8_t* buf, size_t &sz, File::WaitType wait)
{
    ETFW_ASSERT(buf != nullptr, "Read buffer cannot be null");
    ETFW_ASSERT(sz > 0, "Read buffer must be greater than 0");
    ETFW_ASSERT(_mode < File::MAX_OPEN_MODE, "File mode is invalid");

    if (_mode == OPEN_NO_MODE)
    {
        sz = 0;
        return File::NOT_OPENED;
    }
    else if (_mode != OPEN_READ)
    {
        sz = 0;
        return File::NOT_OPENED;
    }

    Status status = File::OP_OK;
    ssize_t read_size = ::read(_fd, buf, sz);
    if (read_size == ERR_RC)
    {
        int _errno = errno;
        status = err_to_status(_errno);
    }
    else if (read_size >= 0)
    {
        sz = static_cast<size_t>(read_size);
    }
    else
    {
        status = File::OTHER_ERROR;
    }

    return status;
}

File::Status File::readline(uint8_t* buf, size_t &sz, File::WaitType wait)
{
    return File::Status::NOT_SUPPORTED;
}

File::Status File::write(uint8_t* buf, size_t &sz, File::WaitType wait)
{
    ETFW_ASSERT(buf != nullptr, "File write buffer cannot be null");
    ETFW_ASSERT(sz >= 0, "File write buffer size must be greater than 0");
    ETFW_ASSERT(_mode < File::Mode::MAX_OPEN_MODE, "Write mode is invalid");

    if (_mode == File::Mode::OPEN_NO_MODE)
    {
        sz = 0;
        return File::Status::NOT_OPENED;
    }
    else if (_mode == File::Mode::OPEN_READ)
    {
        sz = 0;
        return File::Status::INVALID_MODE;
    }

    Status status = File::OP_OK;
    ssize_t write_size = ::write(_fd, buf, sz);
    if (write_size == ERR_RC)
    {
        int _errno = errno;
        status = err_to_status(_errno);
    }
    else if (write_size >= 0)
    {
        sz = static_cast<size_t>(write_size);
    }
    else
    {
        status = File::Status::OTHER_ERROR;
    }

    return status;
}

File::Status File::size(size_t &result)
{
    ETFW_ASSERT(_mode >= 0 && _mode < File::Mode::MAX_OPEN_MODE,
        "Invalid file mode for size check");
    if (_mode == File::Mode::OPEN_NO_MODE)
    {
        return File::Status::NOT_OPENED;
    }

    return File::Status::NOT_SUPPORTED;
}

File::Status File::position(size_t &result)
{
    return File::Status::NOT_SUPPORTED;
}

File::Status File::calc_crc(uint32_t &crc)
{
    return File::Status::NOT_SUPPORTED;
}
