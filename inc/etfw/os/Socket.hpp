
#pragma once

#include <cassert>
#include <cstdint>
#include "etfw/status.hpp"

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
                Port(0)
            {}
            
            Address(const char *addr, uint16_t port):
                Addr(addr),
                Port(port)
            {}
        };

        Sock();

        Sock(AddressDomain domain);

        Sock(Type type);

        Sock(AddressDomain domain, Type type);

        Status open() noexcept;

        Status close() noexcept;

        Status bind(Address &addr) noexcept;

        Status receive(uint8_t* &buf, size_t &sz) noexcept;

        Status send(uint8_t* buf, size_t &sz, Address &address) noexcept;

        inline Os::OsFd_t fd(void) const noexcept { return fd_; }

        inline bool is_open() const { return is_open_; }

        inline bool is_bound() const { return is_bound_; }

        inline Type sock_type() const { return sock_type_; }

        inline AddressDomain addr_domain() const { return domain_; }

        Status set_type(Type sock_type)
        {
            if (!is_open_ &&
                !is_bound_)
            {
                sock_type_ = sock_type;
                return Status::OP_OK;
            }
            return Status::IS_OPEN;
        }

        Status set_domain(AddressDomain sock_domain)
        {
            if (!is_open_ &&
                !is_bound_)
            {
                domain_ = sock_domain;
                return Status::OP_OK;
            }
            return Status::IS_OPEN;
        }

    private:
        Os::OsFd_t fd_;
        Type sock_type_;
        AddressDomain domain_;
        bool is_open_;
        bool is_bound_;
};

} // namespace Os
