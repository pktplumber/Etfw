
#include <etfw/msg/Pkt.hpp>
#include <etfw/msg/Pool.hpp>

using namespace etfw::msg;

Buf::Buf(MsgBufPool& owner, size_t buf_sz):
    owner_(owner),
    ref_count_(1),
    msg_sz_(buf_sz)
{}

void Buf::release()
{
    owner_.release(*this);
}

uint8_t* Buf::data_buf()
{
    return reinterpret_cast<uint8_t*>(this+1);
}

size_t Buf::buf_size()
{
    return msg_sz_;
}
