
#pragma once

#include <etfw/svcs/App.hpp>
#include "etfw/msg/Context.hpp"
#include <etfw/msg/Message.hpp>
#include <etl/array.h>
#include <initializer_list>

/// @brief Very simple message scheduler class
class MsgScheduler
{
public:
    /// @brief Number of scheduler table entries
    static constexpr size_t NumSchedTblEntries = 10;

    /// @brief Milliseconds between each scheduler period
    static constexpr uint32_t SchedPeriodMs = 100;

    /// @brief Message scheduler table entry
    struct Entry
    {
        etfw::msg::MsgId_t MsgId; //< MsgId to send
        size_t WakeupCount;   //< Send message every WakeupCount * SchedPeriodMs ms
        bool InUse; //< Entry in use

        Entry():
            MsgId(0),
            WakeupCount(0),
            InUse(false)
        {}

        Entry(etfw::msg::MsgId_t msg_id, size_t wkup_count):
            MsgId(msg_id),
            WakeupCount(wkup_count),
            InUse(true)
        {}
    };

    /// @brief Message scheduler table type
    using SchedTbl_t = Entry[NumSchedTblEntries];

    MsgScheduler(std::initializer_list<Entry> entries):
        wkup_count_(0)
    {
        std::copy(entries.begin(), entries.end(), sched_tbl_);
    }

    void run()
    {
        usleep(SchedPeriodMs*1000);
        wkup_count_++;
        //printf("Sched woke up. Count = %d\n", wkup_count_);
        for (auto& entry: sched_tbl_)
        {
            //printf("Trying entry ID %X. In use = %d, %d\n", entry.MsgId, entry.InUse, entry.WakeupCount);
            //printf("Remainder = %d\n", entry.WakeupCount % wkup_count_);
            if (entry.InUse &&
                (wkup_count_ % entry.WakeupCount) == 0)
            {
                etfw::msg::iBaseMsg msg(entry.MsgId);
                etfw::msg::glob.send_msg(msg);
                //etfw::iApp::send_cmd(msg);
            }
        }
    }

private:
    size_t wkup_count_;
    SchedTbl_t sched_tbl_;
};
