#ifndef _IPC_CORE_CONNECTION_H_
#define _IPC_CORE_CONNECTION_H_

#include <config.h>
#include <utility>
#include <cstring>
#include <bitset>
#include <assert.h>
#include <ipc/def.h>
#include <sync/Waiter.h>

namespace ipc
{
namespace detail
{

// 高位是receiver,低位是sender
class Connection
{
public:
    Connection()
        :cc_{}
    {
        assert(!(MAX_CONNECTIONS % 2) && "MAX_CONNECTIONS is must be an even number.");
    }

    virtual ~Connection()
    {
        auto guard = std::unique_lock(lcc_);
        cc_.reset();
    }
public:
    virtual uint32_t connect(const unsigned &mode = SENDER) noexcept
    {
        auto guard = std::unique_lock(lcc_);

        uint32_t start_pos = (mode == SENDER) ? 0 : (MAX_CONNECTIONS / 2);
        uint32_t end_pos = (mode == SENDER) ?  (MAX_CONNECTIONS / 2) : MAX_CONNECTIONS;
        uint32_t cur_pos = start_pos;
        for (; cur_pos < end_pos; cur_pos++)
        {
            if(!cc_.test(cur_pos))
            {
                cc_.set(cur_pos);
                break;
            }
        }
        if(cur_pos == end_pos)
        {
            assert("The maximum subscript is exceeded.");
        }
        return cur_pos + 1;
    }

    virtual uint32_t disconnect(const unsigned &mode = SENDER,uint32_t cc_id = 0) noexcept
    {
        auto guard = std::unique_lock(lcc_);
        cc_.reset(cc_id);
        return cc_id;
    }

    virtual uint32_t connections(std::memory_order order = std::memory_order_acquire) noexcept
    {
        auto guard = std::unique_lock(lcc_);
        return cc_.count();
    }

    virtual uint32_t recv_count() noexcept
    {
        auto guard = std::unique_lock(lcc_);
        auto tmp = cc_;
        tmp << (MAX_CONNECTIONS / 2);
        return tmp.count();
    }

protected:
    SpinLock lcc_;
    std::bitset<MAX_CONNECTIONS> cc_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_CORE_CONNECTION_H_