
#include "ut_framework.hpp"
#include <etfw/msg/Broker.hpp>
#include <etfw/msg/Pipe.hpp>
#include <etl/queue.h>

namespace {

// Test message 1
struct M1 : public etfw::msg::iBaseMsg
{
    uint16_t Val;

    M1():
        etfw::msg::iBaseMsg(1)
    {}
};

// Test message 1
struct M2 : public etfw::msg::iBaseMsg
{
    bool Flag;

    M2():
        etfw::msg::iBaseMsg(2)
    {}

    M2(bool flag):
        etfw::msg::iBaseMsg(2),
        Flag(flag)
    {}
};

struct M3 : public etfw::msg::iBaseMsg
{
    M3():
        etfw::msg::iBaseMsg(3)
    {}
};

using msg_init_list = std::initializer_list<etfw::msg::MsgId_t>;

class UtPipe : public etfw::msg::iPipe
{
public:
    using Base_t = etfw::msg::iPipe;

    UtPipe(etfw::msg::iPipe::PipeId_t id):
        Base_t(id),
        MsgsReceived(0),
        LastRxMsgId(etfw::msg::MsgIdRsvd)
    {}

    UtPipe(
        etfw::msg::iPipe::PipeId_t id,
        msg_init_list msg_ids
    ):
        Base_t(id, msg_ids),
        MsgsReceived(0),
        LastRxMsgId(etfw::msg::MsgIdRsvd)
    {}

    inline size_t rx_count() const { return MsgsReceived; }

    inline etfw::msg::MsgId_t last_rx_id() const { return LastRxMsgId; }

protected:
    void update_rx_vars(const etfw::msg::MsgId_t id)
    {
        MsgsReceived++;
        LastRxMsgId = id;
    }

private:
    size_t MsgsReceived;
    etfw::msg::MsgId_t LastRxMsgId;
};

class SimplePipe : public UtPipe
{
public:
    using Base_t = UtPipe;

    SimplePipe():
        Base_t(1)
    {}

    SimplePipe(msg_init_list msg_ids):
        Base_t(1, msg_ids)
    {}

    void receive(const etl::imessage& msg) override
    {
        update_rx_vars(msg.get_message_id());
        printf("Received message %d\n", msg.get_message_id());
    }

private:
};

class QueuedPipe : public UtPipe
{
public:
    using Base_t = UtPipe;

    QueuedPipe():
        Base_t(1)
    {}

    QueuedPipe(msg_init_list msg_ids):
        Base_t(1, msg_ids)
    {}

    void receive(etl::shared_message msg) override
    {
        if (!q_.full())
        {
            q_.push(msg);
        }
    }

    void process_queue()
    {
        while (!q_.empty())
        {
            etl::shared_message sm = q_.front();
            update_rx_vars(sm.get_message().get_message_id());
            q_.pop();
        }
    }

    void receive(const etl::imessage& msg) override
    {
        update_rx_vars(msg.get_message_id());
        printf("Received message %d\n", msg.get_message_id());
    }

    size_t items_queued() const { return q_.size(); }

private:
    etl::queue<etl::shared_message, 5> q_;
};

// ~~~~~~~~~~~~~~~~~~~~ Start tests ~~~~~~~~~~~~~~~~~~~~

TEST(MsgBroker, Init)
{
    etfw::msg::Broker broker;
    SimplePipe pipe1({1, 2});
    SimplePipe pipe2({2});
    QueuedPipe pipe3({3});

    broker.register_pipe(pipe1);
    broker.register_pipe(pipe2);
    broker.register_pipe(pipe3);

    // Test message with single dest
    broker.send<M1>();
    EXPECT_EQ(pipe1.rx_count(), 1);
    EXPECT_EQ(pipe2.rx_count(), 0);
    EXPECT_EQ(pipe3.rx_count(), 0);
    EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
    EXPECT_EQ(broker.pool_stats().AllocCount, 1);
    EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
    EXPECT_EQ(pipe1.last_rx_id(), 1);
    EXPECT_EQ(pipe2.last_rx_id(), 0);

    // Test message that is multi-casted
    broker.send<M2>();
    EXPECT_EQ(pipe1.rx_count(), 2);
    EXPECT_EQ(pipe2.rx_count(), 1);
    EXPECT_EQ(pipe3.rx_count(), 0);
    EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
    EXPECT_EQ(broker.pool_stats().AllocCount, 2);
    EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
    EXPECT_EQ(pipe1.last_rx_id(), 2);
    EXPECT_EQ(pipe2.last_rx_id(), 2);

    // Test message with deferred (queued) processing
    broker.send<M3>();
    EXPECT_EQ(pipe1.rx_count(), 2);
    EXPECT_EQ(pipe2.rx_count(), 1);
    EXPECT_EQ(pipe3.rx_count(), 0);
    EXPECT_EQ(pipe3.items_queued(), 1);
    EXPECT_EQ(broker.pool_stats().ItemsAllocated, 1);
    EXPECT_EQ(broker.pool_stats().AllocCount, 3);
    EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
    pipe3.process_queue();
    EXPECT_EQ(pipe1.rx_count(), 2);
    EXPECT_EQ(pipe2.rx_count(), 1);
    EXPECT_EQ(pipe3.rx_count(), 1);
    EXPECT_EQ(pipe3.items_queued(), 0);
    EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
    EXPECT_EQ(broker.pool_stats().AllocCount, 3);
    EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
    EXPECT_EQ(pipe1.last_rx_id(), 2);
    EXPECT_EQ(pipe2.last_rx_id(), 2);
    EXPECT_EQ(pipe3.last_rx_id(), 3);
}

}
