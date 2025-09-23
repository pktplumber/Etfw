
#include "ut_framework.hpp"

#include <etfw/msg/Pool.hpp>
#include <etfw/msg/MsgBroker.hpp>

using Pool = etfw::msg::MsgBufPool;

TEST(MsgBuf, AllocateRawSize)
{
    Pool pool(10);

    EXPECT_EQ(pool.stats().items_avail(), 10);
    EXPECT_EQ(pool.stats().WaterMark, 0);

    etfw::msg::MsgBuf* buf1 = pool.allocate(static_cast<size_t>(200));
    ASSERT_NE(buf1, nullptr);
    EXPECT_EQ(buf1->buf_size(), 200);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 1);

    etfw::msg::MsgBuf* buf2 = pool.allocate(static_cast<size_t>(64));
    ASSERT_NE(buf2, nullptr);
    EXPECT_EQ(buf2->buf_size(), 64);
    EXPECT_EQ(pool.stats().items_avail(), 8);
    EXPECT_EQ(pool.stats().WaterMark, 2);

    buf1->release();
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 2);
    buf2->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);
    EXPECT_EQ(pool.stats().WaterMark, 2);
}

TEST(MsgBuf, AllocateFromMsg)
{
    struct M1 : public etfw::msg::iBaseMsg
    {
        M1():
            etfw::msg::iBaseMsg(1)
        {}
    };

    struct M2 : public etfw::msg::iBaseMsg
    {
        int Val;

        M2():
            etfw::msg::iBaseMsg(2),
            Val(55)
        {}

        M2(int val):
            etfw::msg::iBaseMsg(2),
            Val(val)
        {}
    };

    Pool pool(10);

    etfw::msg::MsgBuf* buf = pool.allocate<M1>();
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M1));
    EXPECT_EQ(buf->get_message().get_message_id(), 1);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 1);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);

    buf = pool.allocate<M2>();
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M2));
    EXPECT_EQ(buf->get_message().get_message_id(), 2);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    auto concrete_msg1 = buf->get_message_type<M2>();
    EXPECT_EQ(concrete_msg1.Val, 55);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);

    buf = pool.allocate<M2>(22);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M2));
    EXPECT_EQ(buf->get_message().get_message_id(), 2);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    auto concrete_msg2 = buf->get_message_type<M2>();
    EXPECT_EQ(concrete_msg2.Val, 22);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);
}
