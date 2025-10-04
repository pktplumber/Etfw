
#include <etfw/msg/Pkt.hpp>
#include <etfw/msg/Pool.hpp>

using namespace etfw::msg;

RefCount::RefCount():
    Base_t()
{}

RefCount::RefCount(int32_t init_val):
    Base_t()
{
    set_reference_count(init_val);
}

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

size_t Buf::buf_size() const
{
    return msg_sz_;
}
