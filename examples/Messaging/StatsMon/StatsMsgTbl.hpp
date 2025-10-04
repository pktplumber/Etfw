
#pragma once

#include <etfw/msg/Message.hpp>
#include <etfw/status.hpp>
#include <etl/observer.h>
#include <etl/vector.h>

#define MAX_STATS_MON_MSG_COUNT         20

namespace stats_mon
{
    // Msg ID alias
    using MsgId_t = etfw::msg::MsgId_t;

    // Entry added event
    struct EntryAdded
    {
        MsgId_t Id;
    };

    // Entry removed event
    struct EntryRemoved
    {
        MsgId_t Id;
    };

    // Max number of observers
    const size_t MAX_MSG_TBL_OBSERVERS = 5;

    // Msg table observer type
    typedef etl::observer<EntryAdded, EntryRemoved> MsgTblObserver;

    // Observable message table class
    class MsgTbl : public etl::observable<MsgTblObserver, MAX_MSG_TBL_OBSERVERS>
    {
    public:        
        // Operation status configuration
        struct StatusTrait
        {
            // Status codes
            enum class Code : uint32_t
            {
                OK,
                ID_ALREADY_IN_TBL,
                ID_NOT_FOUND,
                TBL_FULL,

                COUNT
            };

            // Status code string representations
            static constexpr StatusStr_t ErrStrLkup[] =
            {
                "Success",
                "ID already exists in table",
                "ID not found in table",
                "Table is full"
            };
        };

        // Operation status type
        using Status = EtfwStatus<StatusTrait>;

        // Message table size
        static constexpr size_t TBL_SZ = MAX_STATS_MON_MSG_COUNT;

        // Message table entry type
        struct Entry
        {
            MsgId_t MsgId;

            Entry():
                MsgId(etfw::msg::MsgModuleIdRsvd)
            {}

            Entry(MsgId_t id):
                MsgId(id)
            {}
        };

        // Build table from initializer list
        /// @param entries Table entries
        MsgTbl(std::initializer_list<Entry> entries):
            msg_tbl_(entries)
        {}

        // Build empty table
        MsgTbl()
        {}

        // Remove an ID from the table. Returns err if 
        ///     ID not found
        /// @param[in] id ID to remove.
        Status remove_id(const MsgId_t id)
        {
            Status stat = Status::Code::OK;
            Entry* entry = find(id);
            if (entry != nullptr)
            {
                entry->MsgId = etfw::msg::MsgModuleIdRsvd;
                EntryRemoved evt = {id};
                notify_observers(evt);
                stat = Status::Code::OK;
            }
            return stat;
        }

        // Add ID to table. Returns err if table full
        ///     or ID already exists
        /// @param id ID to add
        Status add_id(const MsgId_t id)
        {
            Status stat = Status::Code::TBL_FULL;

            // Check if ID exists
            Entry* entry = find(id);
            if (entry == nullptr)
            {
                // ID does not already exist. Find next avail entry
                entry = find_unused_entry();
                if (entry != nullptr)
                {
                    // Tbl not full, found entry. Add id
                    entry->MsgId = id;
                    EntryAdded evt = {id};
                    printf("\n____________Added id? %d\n", evt.Id);
                    notify_observers(evt);
                }
                else
                {
                    // No available entry. Table is full
                    stat = Status::Code::TBL_FULL;
                }
            }
            else
            {
                // ID already exists
                stat = Status::Code::ID_ALREADY_IN_TBL;
            }

            return stat;
        }

    protected:
        virtual void on_data_updated(EntryAdded& evt) {}
        virtual void on_data_updated(EntryRemoved& evt) {}

    private:
        etl::vector<Entry, TBL_SZ> msg_tbl_;

        Entry* find(const MsgId_t id)
        {
            Entry* ret = nullptr;
            for (auto& entry: msg_tbl_)
            {
                if (entry.MsgId == id)
                {
                    ret = &entry;
                    break;
                }
            }
            return ret;
        }

        Entry* find_unused_entry()
        {
            Entry* ret = nullptr;
            for (auto& entry: msg_tbl_)
            {
                if (entry.MsgId != etfw::msg::MsgModuleIdRsvd)
                {
                    ret = &entry;
                    break;
                }
            }
            return ret;
        }
    };
}
