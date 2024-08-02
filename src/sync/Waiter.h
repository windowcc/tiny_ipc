#ifndef _IPC_SYNC_WAITER_H_
#define _IPC_SYNC_WAITER_H_

#include <utility>
#include <string>
#include <atomic>
#include <sync/Mutex.h>
#include <sync/Condition.h>
#include <iostream>

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
    static void init();

    bool valid() const noexcept;

    bool open() noexcept;

    void close() noexcept;

    bool notify() noexcept;

    bool broadcast() noexcept;

    bool quit();

    template <typename F>
    bool wait_for(F &&pred, std::uint64_t tm = static_cast<uint64_t>(TimeOut::INVALID_TIMEOUT)) noexcept
    {
        std::lock_guard<Mutex> guard{ mutex_ };
        while ([this, &pred]
        {
            return std::forward<F>(pred)();
        }())
        
        {
            std::cout << "Start of Wait...." << std::endl;
            if (!cond_.wait(mutex_, tm))
            {
                return false;
            }
            std::cout << "End of Wait...." << std::endl;
        }
        return true;
    }

private:
    Mutex mutex_;
    Condition cond_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_SYNC_WAITER_H_