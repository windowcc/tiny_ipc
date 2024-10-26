#ifndef _IPC_SYNC_CONDITION_H_
#define _IPC_SYNC_CONDITION_H_

#include <cstdint>
#include <Handle.h>
#include <sync/Mutex.h>

namespace ipc {
namespace detail {

class Condition
{
public:
    explicit Condition();
    ~Condition();
public:
    bool init() noexcept;

    void close() noexcept;

    bool wait(Mutex &mtx, std::uint64_t tm = static_cast<uint64_t>(TimeOut::INVALID_TIMEOUT)) noexcept;

    bool notify(Mutex &) noexcept;

    bool broadcast(Mutex &) noexcept;
private:
    pthread_cond_t cond_;
};

} // namespace detail
} // namespace ipc

#endif // !_IPC_SYNC_CONDITION_H_