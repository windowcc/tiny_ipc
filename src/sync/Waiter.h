#ifndef _IPC_SYNC_WAITER_H_
#define _IPC_SYNC_WAITER_H_

#include <utility>
#include <string>
#include <atomic>
#include <sync/Mutex.h>
#include <sync/Condition.h>

namespace ipc
{
namespace detail
{

class Waiter
{
public:
    Waiter();
    ~Waiter();
public:
    bool notify() noexcept;

    bool broadcast() noexcept;

    bool quit();

    template <typename F>
    bool wait_for(F &&pred, std::uint64_t tm = static_cast<uint64_t>(TimeOut::INVALID_TIMEOUT)) noexcept
    {
        std::lock_guard<Mutex> guard{ mutex_ };
        while ([this, &pred]
        {
            return !quit_.load(std::memory_order_relaxed) && std::forward<F>(pred)();
        }())
        
        {
            if (!cond_.wait(mutex_, tm))
                return false;
        }
        return true;
    }

private:
    Condition cond_;
    Mutex mutex_;
    std::atomic<bool> quit_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_SYNC_WAITER_H_