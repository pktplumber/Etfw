
#include "os/Socket.hpp"

using namespace Os;


#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int convert_sock_type_enum(Sock::Type type)
{
    if (type == Sock::Type::STREAM)
    {
        return SOCK_STREAM;
    }
    else
    {
        return SOCK_DGRAM;
    }
}

int convert_sock_domain_enum(Sock::AddressDomain domain)
{
    if (domain == Sock::AddressDomain::IPv4)
    {
        return AF_INET;
    }
    else
    {
        return AF_INET6;
    }
}

Sock::Status Sock::open() noexcept
{
    Sock::Status status = OP_OK;
    Fd = ::socket(convert_sock_domain_enum(Domain),
        convert_sock_type_enum(SockType), 0);
    if (Fd == Os::OS_INVALID_FD)
    {
        status = OPEN_FAILURE;
    }
    else
    {
        IsOpen = true;
    }
    return status;
}

Sock::Status Sock::close() noexcept
{
    if (IsOpen)
    {
        ::close(Fd);
        Fd = Os::OS_INVALID_FD;
    }
    IsOpen = false;
    IsBound = false;
    return Sock::Status::OP_OK;
}

Sock::Status Sock::bind(Sock::Address &addr) noexcept
{
    Sock::Status status = NOT_OPENED;
    if (IsOpen)
    {
        sockaddr_in posix_addr = {};
        posix_addr.sin_family = AF_INET;
        posix_addr.sin_port = htons(addr.Port);
        posix_addr.sin_addr.s_addr = inet_addr(addr.Addr);

        int err = ::bind(Fd, (struct sockaddr *)&posix_addr, sizeof(posix_addr));
        if (err >= 0)
        {
            status = OP_OK;
            IsBound = true;
        }
        else
        {
            status = BIND_FAILURE;
        }
    }
    return status;
}

Sock::Status Sock::receive(uint8_t* &buf, size_t &sz) noexcept
{
    if (!IsOpen)
    {
        return Sock::Status::NOT_OPENED;
    }

    if (!IsBound)
    {
        return Sock::Status::NOT_BOUND;
    }

    ssize_t bytes_rxd = ::recv(Fd, buf, sz, 0);
    if (bytes_rxd >= 0)
    {
        sz = static_cast<size_t>(bytes_rxd);
        return Sock::Status::OP_OK;
    }

    return Sock::Status::RECV_ERR;
}

Sock::Status Sock::send(uint8_t* &buf, size_t &sz, Address &address) noexcept
{
    if (!IsOpen)
    {
        return Sock::Status::NOT_OPENED;
    }

    sockaddr_in dest_addr = {};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(address.Port);
    dest_addr.sin_addr.s_addr = inet_addr(address.Addr);

    ssize_t bytes_tx = ::sendto(Fd, buf, sz, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (bytes_tx >= 0)
    {
        sz = static_cast<size_t>(bytes_tx);
        return Sock::Status::OP_OK;
    }
    return Sock::Status::SEND_ERR;
}