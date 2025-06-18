
#pragma once

#include <cassert>
#include <cstdint>

namespace Os
{

using SockPort_t = uint16_t;

#include "OsTypes.hpp"

class Sock
{
    public:
        enum Status : int32_t
        {
            OP_OK = 0,
            INVALID_ARG = -1,
            NOT_OPENED = -2,
            IS_OPEN = -3,
            OPEN_FAILURE = -4,
            BIND_FAILURE = -5,
            NOT_BOUND = -6,
            RECV_ERR = -7,
            SEND_ERR = -8
        };

        enum Type
        {
            STREAM  = 1,
            DGRAM   = 2,
        };

        enum AddressDomain
        {
            IPv4 = 1,
            IPv6 = 2,
        };

        struct Address
        {
            const char *Addr;
            uint16_t Port;

            Address():
                Addr(nullptr),
                Port(0) {}
            
            Address(const char *addr, uint16_t port):
                Addr(addr),
                Port(port) {}
        };

        Sock():
            Fd(Os::OS_INVALID_FD),
            SockType(DGRAM),
            Domain(IPv4),
            IsOpen(false),
            IsBound(false) {}

        Sock(AddressDomain domain):
            Fd(Os::OS_INVALID_FD),
            SockType(DGRAM),
            Domain(domain),
            IsOpen(false),
            IsBound(false) {}

        Sock(Type type):
            Fd(Os::OS_INVALID_FD),
            SockType(type),
            Domain(IPv4),
            IsOpen(false),
            IsBound(false) {}

        Sock(AddressDomain domain, Type type):
            Fd(Os::OS_INVALID_FD),
            SockType(type),
            Domain(domain),
            IsOpen(false),
            IsBound(false) {}

        Status open() noexcept;

        Status close() noexcept;

        Status bind(Address &addr) noexcept;

        Status receive(uint8_t* &buf, size_t &sz) noexcept;

        Status send(uint8_t* &buf, size_t &sz, Address &address) noexcept;

        inline Os::OsFd_t fd(void) const noexcept { return Fd; }
    private:
        Os::OsFd_t Fd;
        Type SockType;
        AddressDomain Domain;
        bool IsOpen;
        bool IsBound;
};

} // namespace Os
