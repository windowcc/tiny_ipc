#ifndef _IPC_CIRC_ELEMDEF_H_
#define _IPC_CIRC_ELEMDEF_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
#include <mutex>
#include <ipc/def.h>
#include <sync/Waiter.h>

namespace ipc
{
namespace detail
{

template <typename WR>
struct RelationTrait;

template <Relation Rp, Relation Rc, Transmission Ts>
struct RelationTrait<Wr<Rp, Rc, Ts>>
{
    constexpr static bool is_multi_producer = (Rp == Relation::MULTI);
    constexpr static bool is_multi_consumer = (Rc == Relation::MULTI);
    constexpr static bool is_broadcast      = (Ts == Transmission::BROADCAST);
};

template <template <typename> class Policy, typename Flag>
struct RelationTrait<Policy<Flag>> : RelationTrait<Flag>
{
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////  


class ConnHeadBase
{
public:
    void init()
    {
        if (!constructed_.load(std::memory_order_acquire))
        {
            auto guard = std::unique_lock(lc_);
            if (!constructed_.load(std::memory_order_relaxed))
            {
                ::new (this) ConnHeadBase;
                waiter_.init();
                constructed_.store(true, std::memory_order_release);
            }
        }
    }

    ConnHeadBase() = default;
    ConnHeadBase(ConnHeadBase const &) = delete;
    ConnHeadBase &operator=(ConnHeadBase const &) = delete;

    uint32_t connections(std::memory_order order = std::memory_order_acquire) const noexcept
    {
        return this->cc_.load(order);
    }

public:
    Waiter waiter_;
    std::atomic<uint32_t> cc_{0}; // connections
    ipc::detail::SpinLock lc_;
    std::atomic<bool> constructed_{false};
};

template <typename ProdCons, bool = RelationTrait<ProdCons>::is_broadcast>
class Head;

template <typename ProdCons>
class Head<ProdCons, true> : public ConnHeadBase
{
public:
    uint32_t connect() noexcept
    {
        for (unsigned k = 0;; ipc::yield(k))
        {
            uint32_t curr = this->cc_.load(std::memory_order_acquire);
            uint32_t next = curr | (curr + 1); // find the first 0, and set it to 1.
            if (next == 0)
            {
                // connection-slot is full.
                return 0;
            }
            if (this->cc_.compare_exchange_weak(curr, next, std::memory_order_release))
            {
                return next ^ curr; // return connected id
            }
        }
    }

    uint32_t disconnect(uint32_t cc_id) noexcept
    {
        return this->cc_.fetch_and(~cc_id, std::memory_order_acq_rel) & ~cc_id;
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept
    {
        uint32_t cur = this->cc_.load(order);
        uint32_t cnt; // accumulates the total bits set in cc
        for (cnt = 0; cur; ++cnt) cur &= cur - 1;
        return cnt;
    }
};

template <typename ProdCons>
class Head<ProdCons, false> : public ConnHeadBase
{
public:
    uint32_t connect() noexcept
    {
        return this->cc_.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t disconnect(uint32_t cc_id) noexcept
    {
        if (cc_id == ~static_cast<uint32_t>(0u))
        {
            // clear all connections
            this->cc_.store(0, std::memory_order_relaxed);
            return 0u;
        }
        else
        {
            return this->cc_.fetch_sub(1, std::memory_order_relaxed) - 1;
        }
    }

    std::size_t conn_count(std::memory_order order = std::memory_order_acquire) const noexcept
    {
        return this->connections(order);
    }
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_CIRC_ELEMDEF_H_