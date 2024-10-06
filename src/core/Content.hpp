#ifndef _IPC_PRODCONS_H_
#define _IPC_PRODCONS_H_

#include <utility>
#include <cstring>
#include <bitset>
#include <type_traits>
#include <ipc/def.h>
#include <core/Connection.hpp>
#include <sync/RwLock.h>

namespace ipc
{
namespace detail
{

////////////////////////////////////////////////////////////////
/// producer-consumer implementation
////////////////////////////////////////////////////////////////

// Minimum offset between two objects to avoid false sharing.
const static uint8_t Align = 64;

template<typename Wr> 
class Content;

template <>
class Content<Wr<Transmission::UNICAST>> : public Connection<Wr<Transmission::UNICAST>>
{
public:
    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t
    {
        std::aligned_storage_t<DataSize, AlignSize> data_{};
    };

    uint32_t rd() const noexcept
    {
        return r_.load(std::memory_order_acquire);
    }

    uint32_t wr() const noexcept
    {
        return w_.load(std::memory_order_acquire);
    }

    template <typename W, typename F, typename Seg>
    bool push(W * /*wrapper*/, F &&f, Seg *seg)
    {
        auto cur_wt = static_cast<uint8_t>(w_.load(std::memory_order_relaxed));
        if (cur_wt == static_cast<uint8_t>(r_.load(std::memory_order_acquire) - 1))
        {
            return false; // full
        }
        std::forward<F>(f)(&(seg[cur_wt].data_));
        w_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename R, typename Seg>
    bool pop(W * /*wrapper*/, uint32_t &cur, F &&f, R &&out, Seg *seg)
    {
        if (static_cast<uint8_t>(cur) == static_cast<uint8_t>(w_.load(std::memory_order_acquire)))
        {
            return false; // empty
        }
        std::forward<F>(f)(&(seg[static_cast<uint8_t>(cur++)].data_));
        if(std::forward<R>(out)(true))
        {
            r_.fetch_add(1, std::memory_order_release);
        }
        return true;
    }

protected:
    alignas(Align) std::atomic<uint32_t> r_; // read index
    alignas(Align) std::atomic<uint32_t> w_; // write index
};


// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template <>
class Content<Wr<Transmission::BROADCAST>> : public Connection<Wr<Transmission::BROADCAST>>
{
public:
    template <std::size_t DataSize, std::size_t AlignSize>
    struct elem_t
    {
        std::aligned_storage_t<DataSize, AlignSize> data_{};
    };

    uint32_t rd() const noexcept
    {
        return r_.load(std::memory_order_acquire);
    }

    uint32_t wr() const noexcept
    {
        return w_.load(std::memory_order_acquire);
    }

    template <typename W, typename F, typename Seg>
    bool push(W * /*wrapper*/, F &&f, Seg *seg)
    {
        auto cur_wt = static_cast<uint8_t>(w_.load(std::memory_order_relaxed));
        if (cur_wt == static_cast<uint8_t>(r_.load(std::memory_order_acquire) - 1))
        {
            return false; // full
        }
        std::forward<F>(f)(&(seg[cur_wt].data_));
        w_.fetch_add(1, std::memory_order_release);
        return true;
    }

    template <typename W, typename F, typename R, typename Seg>
    bool pop(W * /*wrapper*/, uint32_t &cur, F &&f, R &&out, Seg *seg)
    {
        if (static_cast<uint8_t>(cur) == static_cast<uint8_t>(w_.load(std::memory_order_acquire)))
        {
            return false; // empty
        }
        std::forward<F>(f)(&(seg[static_cast<uint8_t>(cur++)].data_));
        if(std::forward<R>(out)(true))
        {
            r_.fetch_add(1, std::memory_order_release);
        }
        return true;
    }

protected:
    alignas(Align) std::atomic<uint32_t> r_; // read index
    alignas(Align) std::atomic<uint32_t> w_; // write index
};

} // namespace detail
} // namespace ipc


#endif // ! _IPC_PRODCONS_H_