
#include "ut_framework.hpp"
#include <etfw/msg/Pool.hpp>
#include <etfw/msg/BlockingMsgQueue.hpp>
#include <vector>


struct Msg1 : public etl::imessage
{
    Msg1():
        etl::imessage(1)
    {}
};

struct Msg2 : public etl::imessage
{
    int OtherData;

    Msg2(int other):
        etl::imessage(2),
        OtherData(other)
    {}
};

struct Msg3 : public etl::imessage
{
    bool Flag;
    int OtherData;
    uint16_t Val;

    Msg3(bool flag, int other, uint16_t val):
        etl::imessage(3),
        Flag(flag),
        OtherData(other),
        Val(val)
    {}

    Msg3():
        etl::imessage(3),
        Flag(false),
        OtherData(0),
        Val(0)
    {}
};

template <size_t Q_DEPTH>
using PktQueue = etfw::msg::BlockingMsgQueue<etfw::msg::pkt*, Q_DEPTH>;

/*
TEST(MsgPool, RawAcquire)
{
    etfw::msg::Pool pool(10);
    EXPECT_EQ(pool.stats().NumItems, 10);
    EXPECT_EQ(pool.stats().ItemsAllocated, 0);

    auto msg = pool.acquire_raw(100);
    EXPECT_NE(msg, nullptr);
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);
    EXPECT_EQ(msg->size(), 100);

    msg->release();
    EXPECT_EQ(pool.stats().ItemsAllocated, 0);

    std::vector<etfw::msg::ref_counted_msg*> msgs;
    for (size_t i = 0; i < 10; i++)
    {
        etfw::msg::ref_counted_msg* m = pool.acquire_raw(100);
        EXPECT_NE(msg, nullptr);
        EXPECT_EQ(msg->size(), 100);
        EXPECT_EQ(pool.stats().ItemsAllocated, i+1);
        EXPECT_EQ(pool.stats().WaterMark, i+1);
        msgs.push_back(m);
    }

    // Pool depleted
    msg = pool.acquire_raw(100);
    EXPECT_EQ(msg, nullptr);

    // Release all messages
    for (size_t i = 0; i < 10; i++)
    {
        etfw::msg::ref_counted_msg* m = msgs.back();
        m->release();
        EXPECT_EQ(pool.stats().ItemsAllocated, 10 - (i+1));
        EXPECT_EQ(pool.stats().WaterMark, 10);
        msgs.pop_back();
    }
}

TEST(MsgPool, AcquireFromMsg)
{
    etfw::msg::Pool pool(10);
    etfw::msg::pkt* p1 = pool.acquire(Msg1{});
    ASSERT_NE(p1, nullptr);

    Msg1* msg1 = p1->as_p<Msg1>();
    EXPECT_EQ(msg1->get_message_id(), 1);

    etfw::msg::pkt* p2 = pool.acquire<Msg2>(55);
    ASSERT_NE(p1, nullptr);
    Msg2* msg2 = p2->as_p<Msg2>();
    EXPECT_EQ(msg2->get_message_id(), 2);
    EXPECT_EQ(msg2->OtherData, 55);

    EXPECT_EQ(pool.stats().items_avail(), 8);
    EXPECT_EQ(pool.stats().WaterMark, 2);
    EXPECT_EQ(pool.stats().ItemsAllocated, 2);

    p1->release();
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 2);
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);

    p2->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);
    EXPECT_EQ(pool.stats().WaterMark, 2);
    EXPECT_EQ(pool.stats().ItemsAllocated, 0);
}

TEST(MsgPool, AcquireAndQueue)
{
    PktQueue<5> q1;
    PktQueue<2> q2;
    PktQueue<10> q3;

    etfw::msg::Pool pool(10);
    etfw::msg::pkt* p = pool.acquire(Msg1{});

    EXPECT_EQ(pool.stats().WaterMark, 1);
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);

    p->ref_count().set_reference_count(2);
    q1.emplace(p);
    q3.emplace(p);

    etfw::msg::pkt* p_out = nullptr;
    bool result = q1.front(p_out);
    EXPECT_TRUE(result);
    ASSERT_NE(p_out, nullptr);
    EXPECT_EQ(p_out->message_id(), 1);
    p_out->release();
    // msg not fully released, since ref count was 2
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);

    result = q3.front(p_out);
    EXPECT_TRUE(result);
    ASSERT_NE(p_out, nullptr);
    EXPECT_EQ(p_out->message_id(), 1);
    p_out->release();
    // msg fully released
    EXPECT_EQ(pool.stats().ItemsAllocated, 0);

    p = pool.acquire<Msg3>(true, 99, 55);
    ASSERT_NE(p, nullptr);
    p->ref_count().set_reference_count(3);
    q1.emplace(p);
    q2.emplace(p);
    q3.emplace(p);

    etfw::msg::pkt* p_out1 = nullptr;
    etfw::msg::pkt* p_out2 = nullptr;
    etfw::msg::pkt* p_out3 = nullptr;

    result = q1.front(p_out1);
    EXPECT_TRUE(result);
    ASSERT_NE(p_out1, nullptr);
    EXPECT_EQ(p_out1->message_id(), 3);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Flag, true);
    EXPECT_EQ(p_out1->as_p<Msg3>()->OtherData, 99);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Val, 55);
    EXPECT_EQ(p_out1->ref_count().get_reference_count(), 3);
    p_out1->release();
    EXPECT_EQ(p_out1->ref_count().get_reference_count(), 2);
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);

    result = q2.front(p_out2);
    EXPECT_TRUE(result);
    ASSERT_NE(p_out1, nullptr);
    EXPECT_EQ(p_out1->message_id(), 3);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Flag, true);
    EXPECT_EQ(p_out1->as_p<Msg3>()->OtherData, 99);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Val, 55);
    EXPECT_EQ(p_out1->ref_count().get_reference_count(), 2);
    p_out1->release();
    EXPECT_EQ(p_out1->ref_count().get_reference_count(), 1);
    EXPECT_EQ(pool.stats().ItemsAllocated, 1);

    result = q3.front(p_out3);
    EXPECT_TRUE(result);
    ASSERT_NE(p_out1, nullptr);
    EXPECT_EQ(p_out1->message_id(), 3);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Flag, true);
    EXPECT_EQ(p_out1->as_p<Msg3>()->OtherData, 99);
    EXPECT_EQ(p_out1->as_p<Msg3>()->Val, 55);
    EXPECT_EQ(p_out1->ref_count().get_reference_count(), 1);
    p_out1->release();
    EXPECT_EQ(pool.stats().ItemsAllocated, 0);
}
*/