
#include <etfw/msg/Pkt.hpp>
#include <etfw/msg/Pool.hpp>

using namespace etfw::msg;

MsgBuf::MsgBuf(MsgBufPool& owner, size_t buf_sz):
    owner_(owner),
    ref_count_(1),
    msg_sz_(buf_sz)
{
    printf("Called simple constructor %d\n", buf_sz);
}

void MsgBuf::release()
{
    owner_.release(*this);
}

uint8_t* MsgBuf::data_buf()
{
    return reinterpret_cast<uint8_t*>(this+1);
}

size_t MsgBuf::buf_size()
{
    return msg_sz_;
}

pkt::pkt(Pool& pool, size_t sz):
    owner_(pool),
    msg_sz_(sz)
{
    ref_count_.set_reference_count(1);
}

pkt::pkt(Pool& pool, size_t sz, int32_t refs):
    owner_(pool),
    msg_sz_(sz)
{
    ref_count_.set_reference_count(refs);
}

void pkt::release()
{
    owner_.release(*this);
}

uint8_t* pkt::data_buf()
{
    return reinterpret_cast<uint8_t*>(this+1);
}

MsgId_t pkt::message_id()
{
    return as_p<etl::imessage>()->get_message_id();
}

template <typename T>
T* pkt::data()
{
    return static_cast<T*>(this+1);
}
