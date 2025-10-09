
#pragma once

#include <etl/queue_spsc_atomic.h>
#include "os/CountSem.hpp"
#include "etfw_assert.hpp"

namespace etfw {
namespace msg {

template <typename T, size_t QDepth>
class BlockingMsgQueue
{
    static_assert(QDepth <= 255, "Max Q Depth exceeded");

    public:
        /// @brief Default constructor
        /// @details Will attempt to initialize internal semaphore
        BlockingMsgQueue()
        {
            ETFW_ASSERT(Sem.init() == Os::CountSem::Status::OP_OK,
                "Failed to initialize queue semaphore");
        }

        template <typename ... Args>
        bool emplace(Args && ... args)
        {
            if(_queue.emplace(args...))
            {
                Sem.give();
                return true;
            }
            // Queue full
            return false;
        }

        bool push(T& item)
        {
            if (_queue.push(item))
            {
                Sem.give();
                return true;
            }
            return false;
        }

        bool front(T& value)
        {
            if (Sem.take() == Os::CountSem::Status::OP_OK)
            {
                bool result = _queue.front(value);
                ETFW_ASSERT(result, "Sem available but queue is empty");
                _queue.pop();
                return result;
            }
            return false;
        }

        bool front(T& value, const Os::TimeMs_t timeout_ms)
        {
            if (Sem.take(timeout_ms) == Os::CountSem::Status::OP_OK)
            {
                bool result = _queue.front(value);
                ETFW_ASSERT(result, "Sem available but queue is empty");
                _queue.pop();
                return result;
            }
            return false;
        }

        T& front(const Os::TimeMs_t timeout_ms)
        {
            Sem.take(timeout_ms);
            return _queue.front();
        }

        inline bool full() const { return _queue.full(); }

        inline bool empty() const { return _queue.empty(); }

        inline bool pop() { return _queue.pop(); }

    private:
        Os::CountSem Sem;
        etl::queue_spsc_atomic<T, 
            QDepth, etl::memory_model::MEMORY_MODEL_SMALL> _queue;
};

}
}
