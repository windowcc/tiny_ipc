#include <sync/Waiter.h>
#include <sync/Mutex.h>

namespace ipc
{
namespace detail
{

Waiter::Waiter()
    : cond_()
    , mutex_()
{
    // open(name);
}

Waiter::~Waiter()
{
    close();
}

void Waiter::init()
{
}

bool Waiter::valid() const noexcept
{
    return cond_.valid() && mutex_.valid();
}

bool Waiter::open() noexcept
{
    return cond_.init() && mutex_.open();
}

void Waiter::close() noexcept
{
    cond_.close();
    mutex_.close();
}

bool Waiter::notify() noexcept
{
    return cond_.notify(mutex_);
}

bool Waiter::broadcast() noexcept
{
    return cond_.broadcast(mutex_);
}

bool Waiter::quit()
{
    return broadcast();
}

} // namespace detail
} // namespace ipc
