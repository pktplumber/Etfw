
#include <etfw/msg/Broker.hpp>

using namespace etfw::msg;

Broker::Broker():
    msg_pool_(100)
{
    assert(lock_.init().success() &&
        "Failed to initialize broker lock");
}

void Broker::send(const iMsg& msg, const size_t msg_sz)
{
    //SharedMsg msg = SharedMsg::create_from_size(msg_pool_, msg_sz);
}

void Broker::send(Buf& msg_buf)
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
    lock_.unlock();
}
