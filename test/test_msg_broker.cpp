
#include "ut_framework.hpp"
#include <etfw/msg/Broker.hpp>
#include <etfw/msg/Pipe.hpp>
#include <etl/queue.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

// UT Namespace
namespace {

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
        BaseMsg_t(M1_ID, sizeof(M1))
    {}
};

// Test message 1
struct M2 : public BaseMsg_t
{
    bool Flag;

    M2():
        BaseMsg_t(M2_ID, sizeof(M2))
    {}

    M2(bool flag):
        BaseMsg_t(M2_ID, sizeof(M2)),
        Flag(flag)
    {}
};

// Test message 3
struct M3 : public BaseMsg_t
{
    M3():
        BaseMsg_t(M3_ID, sizeof(M3))
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
        Val1(val1),
        Flag(flag),
        Val2(val2)
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
        memset(Contents, 0, sizeof(Contents));
    }

    CM2(std::vector<uint8_t> contents):
        BaseMsg_t(CM2_ID, total_msg_size(contents))
    {
        memset(Contents, 0, sizeof(Contents));
        memcpy(Contents, contents.data(), MsgSize);
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
        LastRxMsgId(etfw::msg::MsgIdRsvd),
        LastRxMsgLen(0)
    {}

    UtPipe(
        etfw::msg::iPipe::PipeId_t id,
        msg_init_list msg_ids
    ):
        Base_t(id, msg_ids),
        MsgsReceived(0),
        LastRxMsgId(etfw::msg::MsgIdRsvd),
        LastRxMsgLen(0)
    {}

    inline size_t rx_count() const { return MsgsReceived; }

    inline MsgId_t last_rx_id() const { return LastRxMsgId; }

    inline size_t last_rx_msg_len() const { return LastRxMsgLen; }

protected:
    void update_rx_vars(const MsgId_t id)
    {
        MsgsReceived++;
        LastRxMsgId = id;
    }

    void update_rx_vars(const BaseMsg_t& msg)
    {
        MsgsReceived++;
        LastRxMsgId = msg.get_message_id();
        LastRxMsgLen = msg.MsgSize;
    }

private:
    size_t MsgsReceived;
    MsgId_t LastRxMsgId;
    size_t LastRxMsgLen;
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
            //update_rx_vars(sm.get_message().get_message_id());
            update_rx_vars(*static_cast<const BaseMsg_t*>(&sm.get_message()));
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

        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);

        // Test message with single dest
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 1);
        EXPECT_EQ(pipe2.rx_count(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        // Test message that is multi-casted
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
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
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 0);
        pipe3.process_queue();
        EXPECT_EQ(pipe3.rx_count(), 1);
        EXPECT_EQ(pipe3.items_queued(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.last_rx_id(), 3);

        // Test pipe overflow
        broker.send<M3>(); // 1
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 1);
        broker.send<M3>(); // 2
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 2);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 2);
        broker.send<M3>(); // 3
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 3);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 3);
        broker.send<M3>(); // 4
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 4);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 4);
        broker.send<M3>(); // 5
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Pipe now full
        broker.send<M3>();
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Start processing pipe
        pipe3.process_queue();
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
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
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        broker.send(m2);
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Test pipe overflow
        broker.send(m3); // 1
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 1);
        broker.send(m3); // 2
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 2);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 2);
        broker.send(m3); // 3
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 3);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 3);
        broker.send(m3); // 4
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 4);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 4);
        broker.send(m3); // 5
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Pipe now full
        broker.send(m3);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 5);
        EXPECT_EQ(broker.pool_stats().AllocCount, 8);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(pipe3.items_queued(), 5);

        // Start processing pipe
        pipe3.process_queue();
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
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

namespace complex_msg {

    static MsgId_t rx_id = 0;
    static size_t msg_len = 0;
    static CM1 last_cm1_msg_rx; // Copy of CM1 message
    static CM2 last_cm2_msg_rx; // Copy of CM2 message

    void rx_callback(const etl::imessage& msg)
    {
        const etfw::msg::iBaseMsg& conv_msg = etfw::msg::convert<etfw::msg::iBaseMsg>(msg);
        
        rx_id = conv_msg.get_message_id();
        msg_len = conv_msg.MsgSize;
        if (rx_id == CM1_ID)
        {
            // Copy message
            last_cm1_msg_rx = conv_msg.convert<CM1>();
        }
        else if (rx_id == CM2_ID)
        {
            last_cm2_msg_rx = conv_msg.convert<CM2>();
        }
    }

    TEST(MsgBroker, ComplexMsg)
    {
        etfw::msg::Broker broker;
        SimplePipe pipe1({CM1_ID});
        SimplePipe pipe2({CM2_ID});
        pipe1.set_rx_hook(rx_callback);
        pipe2.set_rx_hook(rx_callback);
        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);

        broker.send<CM1>(255, true, 99);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), CM1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), 0);
        EXPECT_EQ(rx_id, CM1_ID);
        EXPECT_EQ(msg_len, sizeof(CM1));
        EXPECT_EQ(last_cm1_msg_rx.get_message_id(), CM1_ID);
        EXPECT_EQ(last_cm1_msg_rx.MsgSize, sizeof(CM1));
        EXPECT_EQ(last_cm1_msg_rx.Val1, 255);
        EXPECT_EQ(last_cm1_msg_rx.Flag, true);
        EXPECT_EQ(last_cm1_msg_rx.Val2, 99);

        broker.send<CM2>(std::vector<uint8_t>{1, 2, 3, 4, 5, 6});
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), CM1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), CM2_ID);
        EXPECT_EQ(rx_id, CM2_ID);
        EXPECT_EQ(msg_len, sizeof(BaseMsg_t) + 6);
        // Test conversion
        EXPECT_EQ(last_cm2_msg_rx.MsgSize, sizeof(BaseMsg_t) + 6);
        EXPECT_EQ(last_cm2_msg_rx.get_message_id(), CM2_ID);
        EXPECT_EQ(memcmp(
            last_cm2_msg_rx.Contents,
            std::vector<uint8_t>{1, 2, 3, 4, 5, 6}.data(),
            6
        ), 0) << "Invalid CM2 contents " << to_hex_bytes(last_cm2_msg_rx.Contents);

        broker.send<CM2>(std::vector<uint8_t>{
            0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11
        });
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(pipe1.last_rx_id(), CM1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), CM2_ID);
        EXPECT_EQ(rx_id, CM2_ID);
        EXPECT_EQ(msg_len, sizeof(BaseMsg_t) + 9);
        // Test conversion
        EXPECT_EQ(last_cm2_msg_rx.MsgSize, sizeof(BaseMsg_t) + 9);
        EXPECT_EQ(last_cm2_msg_rx.get_message_id(), CM2_ID);
        EXPECT_EQ(memcmp(
            last_cm2_msg_rx.Contents,
            std::vector<uint8_t>{0x99, 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11}.data(),
            6
        ), 0) << "Invalid CM2 contents " << to_hex_bytes(last_cm2_msg_rx.Contents);
    }
}

namespace allocated_buf {

    static MsgId_t last_rx_id = 0;
    static size_t last_msg_len = 0;
    static M1 last_m1_msg_rx; // Copy of M1 message
    static M2 last_m2_msg_rx; // Copy of M1 message
    static CM2 last_cm2_msg_rx; // Copy of CM2 message

    void rx_callback(const etl::imessage& msg)
    {
        const etfw::msg::iBaseMsg& conv_msg = etfw::msg::convert<etfw::msg::iBaseMsg>(msg);
        
        last_rx_id = conv_msg.get_message_id();
        last_msg_len = conv_msg.MsgSize;
        if (last_rx_id == M1_ID)
        {
            // Copy message
            last_m1_msg_rx = conv_msg.convert<M1>();
        }
        else if (last_rx_id == M2_ID)
        {
            last_m2_msg_rx = conv_msg.convert<M2>();
        }
        else if (last_rx_id == CM2_ID)
        {
            last_cm2_msg_rx = conv_msg.convert<CM2>();
        }
        else
        {
            // Unknown message ID, exit here
            ASSERT_TRUE(0) << "ERROR: Invalid message ID received: " << last_rx_id;
        }
    }

    // Test message broker send with pre-allocated buffer
    // Used for "zero-copy" routing
    TEST(MsgBroker, AllocatedBuf)
    {
        etfw::msg::Broker broker;
        SimplePipe pipe({M1_ID, M2_ID, CM2_ID});
        pipe.set_rx_hook(rx_callback);
        broker.register_pipe(pipe);

        etfw::msg::Buf* buf = broker.get_message_buf(100);
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 0);
        M1 m1;
        memcpy(buf->data(), &m1, sizeof(m1));
        broker.send_buf(*buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(last_rx_id, M1_ID);
        EXPECT_EQ(last_msg_len, sizeof(M1));

        buf = broker.get_message_buf(sizeof(M2));
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        M2 m2(true);
        memcpy(buf->data(), &m2, sizeof(m2));
        broker.send_buf(*buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(last_rx_id, M2_ID);
        EXPECT_EQ(last_msg_len, sizeof(M2));
        EXPECT_TRUE(last_m2_msg_rx.Flag);

        CM2 cm2(std::vector<uint8_t>{
            0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xBA, 0xAD, 0xF0, 0x0D
        });
        buf = broker.get_message_buf(sizeof(CM2));
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        // Copy only the required bytes
        memcpy(buf->data(), &cm2, sizeof(BaseMsg_t) + 9);
        broker.send_buf(*buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(last_rx_id, CM2_ID);
        // Actual message length is the header size + actual contents
        EXPECT_EQ(last_msg_len, sizeof(BaseMsg_t) + 9);
        // Test data contents of static cm2
        EXPECT_EQ(memcmp(
            last_cm2_msg_rx.Contents,
            std::vector<uint8_t>{
                0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0xBA, 0xAD, 0xF0, 0x0D}.data(),
            6
        ), 0) << "Invalid CM2 contents " << 
            to_hex_bytes(last_cm2_msg_rx.Contents);

        // Test release of unused message buffer
        buf = broker.get_message_buf(100);
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        broker.return_message_buf(buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 4);
    }
}

namespace register_unregister
{
    TEST(MsgBroker, RegisterUnregister)
    {
        etfw::msg::Broker broker;
        SimplePipe pipe1({1, 2});
        SimplePipe pipe2({2});

        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);

        // Test message with single dest
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 1);
        EXPECT_EQ(pipe2.rx_count(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        // Test message that is multi-casted
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Send M1 again after unregistering pipe1
        broker.unregister_pipe(pipe1);
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 2) << 
            "Pipe should not have receive message after unregistering";
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Re-register pipe1 and send M1 again
        broker.register_pipe(pipe1);
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 3) << 
            "Pipe should have receive message after re-registration";
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 4);
        EXPECT_EQ(pipe1.last_rx_id(), 1);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Unregister pipe 2 and send M2
        broker.unregister_pipe(pipe2);
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 4) << 
            "Pipe should have receive message after re-registration";
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 5);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Re-register pipe 2 and send M2 again
        broker.register_pipe(pipe2);
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 5) << 
            "Pipe should have receive message after re-registration";
        EXPECT_EQ(pipe2.rx_count(), 2);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 6);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        // Unregister both. And send messages
        // Stats should be the same except for alloc and release counts
        broker.unregister_pipe(pipe1);
        broker.unregister_pipe(pipe2);
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 5);
        EXPECT_EQ(pipe2.rx_count(), 2);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 7);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 7);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);

        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 5);
        EXPECT_EQ(pipe2.rx_count(), 2);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 8);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 8);
        EXPECT_EQ(pipe1.last_rx_id(), 2);
        EXPECT_EQ(pipe2.last_rx_id(), 2);
    }
}

namespace subscription_updates
{
    TEST(MsgBroker, SubscriptionUpdates)
    {
        etfw::msg::Broker broker;
        // Pipe 1 subscribed to several messages at start
        SimplePipe pipe1({1, 2, 3});
        // Pipe 2 constructed with empty subscription
        SimplePipe pipe2;

        broker.register_pipe(pipe1);
        broker.register_pipe(pipe2);

        EXPECT_EQ(broker.stats().RegisteredPipes, 2);

        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 1);
        EXPECT_EQ(pipe2.rx_count(), 0);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe1.last_rx_id(), M1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), 0);

        // Update pipe2 and check if receives the new message
        pipe2.subscribe(M1_ID);
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 2);
        EXPECT_EQ(pipe2.rx_count(), 1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe1.last_rx_id(), M1_ID);
        EXPECT_EQ(pipe2.last_rx_id(), M1_ID);

        pipe2.subscribe(M3_ID);
        broker.send<M3>();
        EXPECT_EQ(pipe1.rx_count(), 3);
        EXPECT_EQ(pipe2.rx_count(), 2);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 3);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 3);
        EXPECT_EQ(pipe1.last_rx_id(), M3_ID);
        EXPECT_EQ(pipe2.last_rx_id(), M3_ID);

        // Unsubscribe pipe1 from M1 and make sure the message isn't received
        pipe1.unsubscribe(M1_ID);
        broker.send<M1>();
        EXPECT_EQ(pipe1.rx_count(), 3);
        EXPECT_EQ(pipe2.rx_count(), 3);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 4);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 4);
        EXPECT_EQ(pipe1.last_rx_id(), M3_ID);
        EXPECT_EQ(pipe2.last_rx_id(), M1_ID);

        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 4);
        EXPECT_EQ(pipe2.rx_count(), 3);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 5);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 5);
        EXPECT_EQ(pipe1.last_rx_id(), M2_ID);
        EXPECT_EQ(pipe2.last_rx_id(), M1_ID);

        pipe1.unsubscribe(M2_ID);
        broker.send<M2>();
        EXPECT_EQ(pipe1.rx_count(), 4);
        EXPECT_EQ(pipe2.rx_count(), 3);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 6);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 6);
        EXPECT_EQ(pipe1.last_rx_id(), M2_ID);
        EXPECT_EQ(pipe2.last_rx_id(), M1_ID);
    }
}

namespace dynamic_msg
{
    TEST(MsgBroker, DynamicMsg)
    {
        etfw::msg::Broker broker;
        QueuedPipe pipe;
        EXPECT_EQ(broker.stats().RegisteredPipes, 0);
        broker.register_pipe(pipe);
        EXPECT_EQ(broker.stats().RegisteredPipes, 1);
        EXPECT_EQ(pipe.items_queued(), 0);

        BaseMsg_t ut_msg1(0xAA, 240);
        etfw::msg::Buf* buf = broker.get_message_buf(1024);
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 0);
        memcpy(buf->data(), &ut_msg1, 240);
        broker.send_buf(*buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 1);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe.items_queued(), 0);

        pipe.subscribe(0xAA);
        buf = broker.get_message_buf(1024);
        ASSERT_NE(buf, nullptr);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        memcpy(buf->data(), &ut_msg1, 240);
        broker.send_buf(*buf);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 1);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 1);
        EXPECT_EQ(pipe.items_queued(), 1);
        pipe.process_queue(1);
        EXPECT_EQ(broker.pool_stats().ItemsInUse, 0);
        EXPECT_EQ(broker.pool_stats().AllocCount, 2);
        EXPECT_EQ(broker.pool_stats().ReleaseCount, 2);
        EXPECT_EQ(pipe.rx_count(), 1);
        EXPECT_EQ(pipe.last_rx_id(), 0xAA);
        EXPECT_EQ(pipe.last_rx_msg_len(), 240);
    }
}

}
