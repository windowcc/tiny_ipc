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

template<typename Wr> 
class Connection;

// 高位 RECEIVER
// 低位 SENDER



// unicast
template<>
class Connection<Wr<Transmission::UNICAST>>
{
public:
    Connection()
        :cc_{}
    {
        assert(!(TINYIPC_MAX_CNT % 2) && "TINYIPC_MAX_CNT is must be an even number.");
    }

    virtual ~Connection()
    {
        auto guard = std::unique_lock(lcc_);
        cc_.reset();
    }
public:
    virtual  uint32_t connect(const unsigned &mode = SENDER) noexcept
    {
        auto guard = std::unique_lock(lcc_);

        uint32_t start_pos = 0;
        if(mode == RECEIVER)
        {
            if(cc_[TINYIPC_MAX_CNT - 1])
            {
                return 0;
            }
            start_pos = TINYIPC_MAX_CNT;
        }
        else
        {
            for (uint32_t i = 1; i < TINYIPC_MAX_CNT; i++)
            {
                if(cc_[i - 1] == 0)
                {
                    start_pos = i;
                    break;
                }
            }
        }        
        cc_[start_pos - 1] = 1;
        return start_pos;
    }

    virtual uint32_t disconnect(const unsigned &mode = SENDER,uint32_t cc_id = 0) noexcept
    {
        auto guard = std::unique_lock(lcc_);
        cc_[cc_id] = 0;
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
        return cc_[TINYIPC_MAX_CNT - 1 ] ? 1 : 0;
    }
protected:
    SpinLock lcc_;
    std::bitset<TINYIPC_MAX_CNT> cc_;
};


// brocast
template<>
class Connection<Wr<Transmission::BROADCAST>> : public Connection<Wr<Transmission::UNICAST>>
{
public:
    Connection()
        : Connection<Wr<Transmission::UNICAST>>{}
    {

    }
    virtual ~Connection()
    {
        
    }
public:
    virtual uint32_t connect(const unsigned &mode = SENDER) noexcept final
    {
        auto guard = std::unique_lock(lcc_);
        uint32_t pos = 0;
        uint32_t start_pos = mode == SENDER ? 1 : TINYIPC_MAX_CNT / 2;
        uint32_t end_pos = mode == SENDER ? TINYIPC_MAX_CNT / 2 : TINYIPC_MAX_CNT;

        for (uint32_t i = start_pos; i < end_pos; i++)
        {
            if(cc_[i - 1] == 0)
            {
                pos = i;
                break;
            }
        }
        if(pos)
        {
            cc_[pos - 1] = 1;
        }
        return pos;
    }

    virtual uint32_t recv_count() noexcept final
    {
        auto guard = std::unique_lock(lcc_);
        return (cc_ << (TINYIPC_MAX_CNT / 2)).count();        
    }

};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_CORE_CONNECTION_H_