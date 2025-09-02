
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

Sock::Sock():
    fd_(Os::OS_INVALID_FD),
    sock_type_(DGRAM),
    domain_(IPv4),
    is_open_(false),
    is_bound_(false)
{}

Sock::Sock(AddressDomain domain):
    fd_(Os::OS_INVALID_FD),
    sock_type_(DGRAM),
    domain_(domain),
    is_open_(false),
    is_bound_(false)
{}

Sock::Sock(Type type):
    fd_(Os::OS_INVALID_FD),
    sock_type_(type),
    domain_(IPv4),
    is_open_(false),
    is_bound_(false)
{}

Sock::Sock(AddressDomain domain, Type type):
    fd_(Os::OS_INVALID_FD),
    sock_type_(type),
    domain_(domain),
    is_open_(false),
    is_bound_(false)
{}

Sock::Status Sock::open() noexcept
{
    Sock::Status status = OP_OK;
    fd_ = ::socket(convert_sock_domain_enum(domain_),
        convert_sock_type_enum(sock_type_), 0);
    if (fd_ == Os::OS_INVALID_FD)
    {
        status = OPEN_FAILURE;
    }
    else
    {
        is_open_ = true;
    }
    return status;
}

Sock::Status Sock::close() noexcept
{
    if (is_open_)
    {
        ::close(fd_);
        fd_ = Os::OS_INVALID_FD;
    }
    is_open_ = false;
    is_bound_ = false;
    return Sock::Status::OP_OK;
}

Sock::Status Sock::bind(Sock::Address &addr) noexcept
{
    Sock::Status status = NOT_OPENED;
    if (is_open_)
    {
        sockaddr_in posix_addr = {};
        posix_addr.sin_family = AF_INET;
        posix_addr.sin_port = htons(addr.Port);
        posix_addr.sin_addr.s_addr = inet_addr(addr.Addr.data());

        int err = ::bind(fd_, (struct sockaddr *)&posix_addr, sizeof(posix_addr));
        if (err >= 0)
        {
            status = OP_OK;
            is_bound_ = true;
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
    if (!is_open_)
    {
        return Sock::Status::NOT_OPENED;
    }

    if (!is_bound_)
    {
        return Sock::Status::NOT_BOUND;
    }

    ssize_t bytes_rxd = ::recv(fd_, buf, sz, 0);
    if (bytes_rxd >= 0)
    {
        sz = static_cast<size_t>(bytes_rxd);
        return Sock::Status::OP_OK;
    }

    return Sock::Status::RECV_ERR;
}

Sock::Status Sock::send(uint8_t* buf, size_t &sz, Address &address) noexcept
{
    if (!is_open_)
    {
        return Sock::Status::NOT_OPENED;
    }

    if (buf == nullptr)
    {
        return Sock::Status::INVALID_ARG;
    }

    sockaddr_in dest_addr = {};
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(address.Port);
    dest_addr.sin_addr.s_addr = inet_addr(address.Addr.data());

    ssize_t bytes_tx = ::sendto(fd_, buf, sz, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (bytes_tx >= 0)
    {
        sz = static_cast<size_t>(bytes_tx);
        return Sock::Status::OP_OK;
    }
    return Sock::Status::SEND_ERR;
}
