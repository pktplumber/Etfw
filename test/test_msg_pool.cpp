
#include "ut_framework.hpp"

#include <etfw/msg/Pool.hpp>
#include <etfw/msg/Broker.hpp>

namespace
{

using Pool = etfw::msg::MsgBufPool;

TEST(MsgBuf, AllocateRawSize)
{
    Pool pool(10);

    EXPECT_EQ(pool.stats().items_avail(), 10);
    EXPECT_EQ(pool.stats().WaterMark, 0);

    etfw::msg::Buf* buf1 = pool.allocate(static_cast<size_t>(200));
    ASSERT_NE(buf1, nullptr);
    EXPECT_EQ(buf1->buf_size(), 200);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 1);

    etfw::msg::Buf* buf2 = pool.allocate(static_cast<size_t>(64));
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

    struct M3 : public etfw::msg::iBaseMsg
    {
        bool Flag;
        uint8_t Data[20];
        size_t NumBytes;

        M3():
            etfw::msg::iBaseMsg(3),
            Flag(false),
            NumBytes(0)
        {}

        M3(bool flag, std::vector<uint8_t> data):
            etfw::msg::iBaseMsg(3),
            Flag(flag),
            NumBytes(data.size() < 20 ? data.size() : 20)
        {
            memcpy(Data, data.data(), NumBytes);
        }

        void ut_comp(std::vector<uint8_t> expected)
        {
            size_t len = expected.size();
            ASSERT_LE(len, 20);

            for (size_t i = 0; i < len; ++i)
            {
                EXPECT_EQ(Data[i], expected[i]);
            }

            for (size_t i = len; i < 20; ++i)
            {
                EXPECT_EQ(Data[i], 0);
            }
        }
    };

    Pool pool(10);

    // Allocate message of type with no constructor args
    etfw::msg::Buf* buf = pool.allocate<M1>();
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M1));
    EXPECT_EQ(buf->get_message().get_message_id(), 1);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    EXPECT_EQ(pool.stats().WaterMark, 1);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);

    // Allocate message of type with default constructor
    buf = pool.allocate<M2>();
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M2));
    EXPECT_EQ(buf->get_message().get_message_id(), 2);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    auto concrete_msg1 = buf->get_message_type<M2>();
    EXPECT_EQ(concrete_msg1.Val, 55);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);

    // Allocate message of same type with constructor arg
    buf = pool.allocate<M2>(22);
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M2));
    EXPECT_EQ(buf->get_message().get_message_id(), 2);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    auto concrete_msg2 = buf->get_message_type<M2>();
    EXPECT_EQ(concrete_msg2.Val, 22);
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 10);

    // Allocate message with more complex constructor
    buf = pool.allocate<M3>(true, std::vector<uint8_t>{
        0x01, 0x02, 0x03, 0x04, 0x05});
    ASSERT_NE(buf, nullptr);
    EXPECT_EQ(buf->buf_size(), sizeof(M3));
    EXPECT_EQ(buf->get_message().get_message_id(), 3);
    EXPECT_EQ(pool.stats().items_avail(), 9);
    auto concrete_msg3 = buf->get_message_type<M3>();
    EXPECT_EQ(concrete_msg3.Flag, true);
    EXPECT_EQ(concrete_msg3.NumBytes, 5);
    concrete_msg3.ut_comp({1, 2, 3, 4, 5});

    // Construct from copy
    etfw::msg::Buf* buf2 = pool.allocate(concrete_msg3);
    ASSERT_NE(buf2, nullptr);
    EXPECT_NE(buf, buf2) << "Allocated buf addresses are the same";
    EXPECT_EQ(buf2->buf_size(), sizeof(M3));
    EXPECT_EQ(buf2->get_message().get_message_id(), 3);
    EXPECT_EQ(pool.stats().items_avail(), 8);
    auto concrete_copy = buf2->get_message_type<M3>();
    EXPECT_EQ(concrete_copy.Flag, true);
    EXPECT_EQ(concrete_copy.NumBytes, 5);
    concrete_copy.ut_comp({1, 2, 3, 4, 5});

    // Release original buf and copied message buf
    buf->release();
    EXPECT_EQ(pool.stats().items_avail(), 9) << "og buf not released";
    buf2->release();
    EXPECT_EQ(pool.stats().items_avail(), 10) << "Buf from copy not released";
}

}
