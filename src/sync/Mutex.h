#ifndef _IPC_SYNC_MUTEX_H_
#define _IPC_SYNC_MUTEX_H_

#include <atomic>
#include <cstdint>
#include <mutex>
#include <system_error>
#include <ipc/def.h>
#include <Handle.h>

namespace ipc
{
namespace detail
{

class Mutex
{
public:
    explicit Mutex();
    ~Mutex();

    bool init() noexcept;

    void close() noexcept;

    pthread_mutex_t *native() noexcept;

    bool lock(uint64_t tm = static_cast<uint64_t>(TimeOut::INVALID_TIMEOUT)) noexcept;

    bool try_lock() noexcept(false);

    bool unlock() noexcept;
private:

private:
    pthread_mutex_t mutex_;
};

} // namespace detail
} // namespace ipc

#endif // _IPC_SYNC_MUTEX_H_