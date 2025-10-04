
#include <etfw/msg/Broker.hpp>

using namespace etfw::msg;

Broker::Stats::Stats():
    RegisteredPipes(0),
    NumSendCalls(0),
    AllocateFailures(0)
{}

Broker::Broker():
    msg_pool_(100)
{
    assert(lock_.init().success() &&
        "Failed to initialize broker lock");
}

void Broker::send_buf(Buf& msg_buf)
{
    if (msg_buf.buf_size() >= sizeof(iBaseMsg))
    {
        SharedMsg sm(msg_buf);
        lock_.lock();
        receive(sm);
        lock_.unlock();
    }
    else
    {
        // msg buffer is invalid
    }
}

void Broker::register_pipe(iPipe& pipe)
{
    lock_.lock();
    subscribe(pipe.subs());
    stats_.RegisteredPipes++;
    lock_.unlock();
}

void Broker::unregister_pipe(iPipe& pipe)
{
    lock_.lock();
    unsubscribe(pipe);
    stats_.RegisteredPipes--;
    lock_.unlock();
}

Buf* Broker::get_message_buf(const size_t buf_sz)
{
    // The message pool has an internal lock, no need to lock here
    return msg_pool_.allocate(buf_sz);
}

void Broker::return_message_buf(Buf* buf)
{
    if (buf != nullptr)
    {
        // Pool should determine if buffer is actually owned by this broker
        msg_pool_.release(buf);
    }
}
