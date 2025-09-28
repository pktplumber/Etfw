
#include "ut_framework.hpp"
#include <etfw/msg/Broker.hpp>
#include <etfw/msg/Pipe.hpp>
#include <etl/queue.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

using BaseMsg_t = etfw::msg::iBaseMsg;
using MsgId_t = etfw::msg::MsgId_t;

template <typename T>
std::string to_hex_bytes(const T& obj)
{
    static_assert(std::is_trivially_copyable<T>::value,
        "Type must be trivially copyable");

    const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&obj);
    size_t sz = sizeof(T);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < sz; ++i)
    {
        oss << std::setw(2) << static_cast<int>(byte_ptr[i]);
        if (i + 1 != sz)
            oss << ' ';
    }

    return oss.str();
}

enum MsgIdVals : MsgId_t
{
    M1_ID = 1,
    M2_ID,
    M3_ID,
    CM1_ID,
    CM2_ID,
};

// Test message 1
struct M1 : public BaseMsg_t
{
    uint16_t Val;

    M1():
        BaseMsg_t(M1_ID)
    {}
};

// Test message 1
struct M2 : public BaseMsg_t
{
    bool Flag;

    M2():
        BaseMsg_t(M2_ID)
    {}

    M2(bool flag):
        BaseMsg_t(M2_ID),
        Flag(flag)
    {}
};

// Test message 3
struct M3 : public BaseMsg_t
{
    M3():
        BaseMsg_t(M3_ID)
    {}
};

// Complex test message 1
struct CM1 : public BaseMsg_t
{
    int Val1;
    bool Flag;
    uint16_t Val2;

    CM1():
        BaseMsg_t(CM1_ID, sizeof(CM1)),
        Val1(0),
        Flag(false),
        Val2(0)
    {}

    CM1(int val1, bool flag, uint16_t val2):
        BaseMsg_t(CM1_ID, sizeof(CM1)),
        Val1(0),
        Flag(false),
        Val2(0)
    {}
};

// Complex message type 2. Stores an arbitrary array of data
// MsgSize is the size of the base message + number of valid
// bytes in "Content"
struct CM2 : public BaseMsg_t
{
    static size_t total_msg_size(std::vector<uint8_t>& input_vec)
    {
        size_t content_sz = (input_vec.size() <= 20 ? input_vec.size() : sizeof(Contents));
        return sizeof(BaseMsg_t) + content_sz;
    }

    uint8_t Contents[20];

    CM2():
        BaseMsg_t(CM2_ID, sizeof(BaseMsg_t))
    {
        printf("Constructing message of size %d\n", MsgSize);
        memset(Contents, 0, sizeof(Contents));
        printf("  Contents = %s\n", to_hex_bytes(Contents).data());
    }

    CM2(std::vector<uint8_t> contents):
        BaseMsg_t(CM2_ID, total_msg_size(contents))
    {
        memset(Contents, 0, sizeof(Contents));
        printf("Constructing message of size %d\n", MsgSize);
        memcpy(Contents, contents.data(), MsgSize);
        printf(" Contents = %s\n", to_hex_bytes(Contents).data());
    }
};

using msg_init_list = std::initializer_list<MsgId_t>;

typedef void (*rx_callback)(const etl::imessage&);

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

    inline MsgId_t last_rx_id() const { return LastRxMsgId; }

protected:
    void update_rx_vars(const MsgId_t id)
    {
        MsgsReceived++;
        LastRxMsgId = id;
    }

private:
    size_t MsgsReceived;
    MsgId_t LastRxMsgId;
};

class SimplePipe : public UtPipe
{
public:
    using Base_t = UtPipe;

    SimplePipe():
        Base_t(1),
        last_rx_msg_size(0)
    {}

    SimplePipe(msg_init_list msg_ids):
        Base_t(1, msg_ids),
        last_rx_msg_size(0)
    {}

    void receive(const etl::imessage& msg) override
    {
        update_rx_vars(msg.get_message_id());
        if (rx_hook != nullptr)
        {
            rx_hook(msg);
        }
        printf("Received message %d\n", msg.get_message_id());
    }

    inline void set_rx_hook(rx_callback cb) { rx_hook = cb; }

private:
    size_t last_rx_msg_size;
    rx_callback rx_hook = nullptr;
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

    void process_queue(size_t items_to_process)
    {
        while (!q_.empty() && items_to_process > 0)
        {
            etl::shared_message sm = q_.front();
            update_rx_vars(sm.get_message().get_message_id());
            q_.pop();
            --items_to_process;
        }
    }

    void process_queue()
    {
        process_queue(q_.capacity());
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

namespace {

    TEST(MsgBroker, SimplePipe)
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
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        // Test message that is multi-casted
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);
    }
}

namespace {

    TEST(MsgBroker, QueuedMsgs)
    {
        // Setup
        etfw::msg::Broker broker;
        QueuedPipe pipe3({3});
        broker.register_pipe(pipe3);

        // Queue 1 message
        broker.send<M3>();
        EXPECT_EQ(pipe3.rx_count(), 0);
        EXPECT_EQ(pipe3.items_queued(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 0);
        pipe3.process_queue();
        EXPECT_EQ(pipe3.rx_count(), 1);
        EXPECT_EQ(pipe3.items_queued(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.last_rx_id(), 3);

        // Test pipe overflow
        broker.send<M3>(); // 1
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 1);
        broker.send<M3>(); // 2
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 2);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 2);
        broker.send<M3>(); // 3
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 3);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 3);
        broker.send<M3>(); // 4
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 4);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 4);
        broker.send<M3>(); // 5
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Pipe now full
        broker.send<M3>();
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Start processing pipe
        pipe3.process_queue();
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 7);
        EXPECT_EQ(pipe3.items_queued(), 0);
        EXPECT_EQ(pipe3.last_rx_id(), 3);
    }
}

namespace {

    TEST(MsgBroker, RouteByReference)
    {
        // Setup
        etfw::msg::Broker broker;
        SimplePipe pipe1({1, 2});
        SimplePipe pipe2({2});
        QueuedPipe pipe3({3});

        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);
        broker.register_pipe(pipe3);

        M1 m1;
        M2 m2;
        M3 m3;

        broker.send(m1);
        EXPECT_EQ(pipe1.rx_count(), 1);
        EXPECT_EQ(pipe2.rx_count(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        broker.send(m2);
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Test pipe overflow
        broker.send(m3); // 1
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 1);
        broker.send(m3); // 2
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 2);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 2);
        broker.send(m3); // 3
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 3);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 3);
        broker.send(m3); // 4
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 4);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 4);
        broker.send(m3); // 5
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Pipe now full
        broker.send(m3);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 8);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Start processing pipe
        pipe3.process_queue();
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 8);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 8);
        EXPECT_EQ(pipe3.items_queued(), 0);
        EXPECT_EQ(pipe3.last_rx_id(), 3);

        // Check other pipe stats just to be sure
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);
    }
}

namespace {

    static MsgId_t rx_id = 0;
    static size_t msg_len = 0;
    CM1 last_cm1_msg_rx; // Copy of CM1 message
    CM2 last_cm2_msg_rx; // Copy of CM2 message

    void ComplexMsg_cb(const etl::imessage& msg)
    {
        const etfw::msg::iBaseMsg conv_msg = etfw::msg::convert(msg);
        rx_id = conv_msg.get_message_id();
        msg_len = conv_msg.MsgSize;
        if (rx_id == CM1_ID)
        {
            // Copy message
            last_cm1_msg_rx = conv_msg.convert<CM1>();
            printf("Got cm1 %d %d\n", conv_msg.convert<CM1>().Val1, conv_msg.convert<CM1>().Val2);
        }
        else if (rx_id == CM2_ID)
        {
            CM2 cm2 = conv_msg.convert<CM2>();
            printf("CALLBACK  Contents = %s\n", to_hex_bytes(cm2.Contents).data());
            last_cm2_msg_rx = conv_msg.convert<CM2>();
        }
    }

    TEST(MsgBroker, ComplexMsg)
    {
        etfw::msg::Broker broker;
        SimplePipe pipe1({CM1_ID});
        SimplePipe pipe2({CM2_ID});
        pipe1.set_rx_hook(ComplexMsg_cb);
        pipe2.set_rx_hook(ComplexMsg_cb);
        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);

        broker.send<CM1>(255, true, 99);
        EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), CM1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), 0);
        EXPECT_EQ(rx_id, CM1_ID);
        EXPECT_EQ(msg_len, sizeof(CM1));
        EXPECT_EQ(last_cm1_msg_rx.get_message_id(), CM1_ID);
        EXPECT_EQ(last_cm1_msg_rx.MsgSize, sizeof(CM1));
        //EXPECT_EQ(last_cm1_msg_rx.Val1, 255);
        //EXPECT_EQ(last_cm1_msg_rx.Flag, true);
        //EXPECT_EQ(last_cm1_msg_rx.Val2, 99);

        //broker.send<CM2>(std::vector<uint8_t>{1, 2, 3, 4, 5, 6});
        //EXPECT_EQ(broker.pool_stats().ItemsAllocated, 0);
        //EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        //EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        //EXPECT_EQ(pipe1.last_rx_id(), 0);
        //EXPECT_EQ(pipe2.last_rx_id(), CM2_ID);
        //EXPECT_EQ(rx_id, CM2_ID);
        //EXPECT_EQ(msg_len, sizeof(BaseMsg_t) + 6);
        //// Test conversion
        //EXPECT_EQ(last_cm2_msg_rx.MsgSize, sizeof(BaseMsg_t) + 6);
        //EXPECT_EQ(last_cm2_msg_rx.get_message_id(), CM2_ID);
        //EXPECT_EQ(memcmp(
        //    last_cm2_msg_rx.Contents,
        //    std::vector<uint8_t>{1, 2, 3, 4, 5, 6}.data(),
        //    6
        //), 0) << "Invalid CM2 contents " << to_hex_bytes(last_cm2_msg_rx.Contents);
//
        //broker.send<CM2>(std::vector<uint8_t>{
        //    0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
        //});
    }
}
