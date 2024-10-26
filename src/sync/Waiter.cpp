#include <sync/Waiter.h>
#include <sync/Mutex.h>

namespace ipc
{
namespace detail
{

Waiter::Waiter()
    : cond_()
    , mutex_()
    , quit_(false)
{
}

Waiter::~Waiter()
{

}

bool Waiter::notify() noexcept
{
    {
        std::lock_guard<Mutex> barrier{ mutex_ };
    }
    return cond_.notify(mutex_);
}

bool Waiter::broadcast() noexcept
{
    {
        std::lock_guard<Mutex> barrier{ mutex_ };
    }
    return cond_.broadcast(mutex_);
}

bool Waiter::quit()
{
    quit_.store(true, std::memory_order_release);
    return broadcast();
}

} // namespace detail
} // namespace ipc
