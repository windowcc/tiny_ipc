#ifndef _IPC_PRODCONS_H_
#define _IPC_PRODCONS_H_

#include <utility>
#include <cstring>
#include <type_traits>
#include <ipc/def.h>
#include <sync/RwLock.h>

namespace ipc
{

////////////////////////////////////////////////////////////////
/// producer-consumer implementation
////////////////////////////////////////////////////////////////

// Minimum offset between two objects to avoid false sharing.
const static uint8_t Align = 64;

template <typename Wr>
class ProdCons;

template <>
class ProdCons<Wr<Relation::SINGLE, Relation::SINGLE, Transmission::UNICAST>>
{
public:
template <std::size_t DataSize, std::size_t AlignSize>
struct elem_t
{
    std::aligned_storage_t<DataSize, AlignSize> data_{};
};

constexpr uint32_t cursor() const noexcept
{
    return 0;
}

template <typename W, typename F, typename E>
bool push(W * /*wrapper*/, F &&f, E *elems)
{
    auto cur_wt = static_cast<uint8_t>(w_.load(std::memory_order_relaxed));
    if (cur_wt == static_cast<uint8_t>(r_.load(std::memory_order_acquire) - 1))
    {
        return false; // full
    }
    std::forward<F>(f)(&(elems[cur_wt].data_));
    w_.fetch_add(1, std::memory_order_release);
    return true;
}

template <typename W, typename F, typename R, typename E>
bool pop(W * /*wrapper*/, uint32_t & /*cur*/, F &&f, R &&out, E *elems)
{
    auto cur_rd = static_cast<uint8_t>(r_.load(std::memory_order_relaxed));
    if (cur_rd == static_cast<uint8_t>(w_.load(std::memory_order_acquire)))
    {
        return false; // empty
    }
    std::forward<F>(f)(&(elems[cur_rd].data_));
    std::forward<R>(out)(true);
    r_.fetch_add(1, std::memory_order_release);
    return true;
}

protected:
    alignas(Align) std::atomic<uint32_t> r_; // read index
    alignas(Align) std::atomic<uint32_t> w_; // write index
};

template <>
class ProdCons<Wr<Relation::MULTI, Relation::SINGLE, Transmission::UNICAST>>
: public ProdCons<Wr<Relation::SINGLE, Relation::SINGLE, Transmission::UNICAST>>
{
public:
template <std::size_t DataSize, std::size_t AlignSize>
struct elem_t
{
    std::aligned_storage_t<DataSize, AlignSize> data_{};
    std::atomic<uint64_t> c_{0}; // commit flag
};

template <typename W, typename F, typename E>
bool push(W * /*wrapper*/, F &&f, E *elems)
{
    uint32_t cur_ct, nxt_ct;
    for (unsigned k = 0;;)
    {
        cur_ct = ct_.load(std::memory_order_relaxed);
        if (static_cast<uint8_t>(nxt_ct = cur_ct + 1) ==
            static_cast<uint8_t>(r_.load(std::memory_order_acquire)))
        {
            return false; // full
        }
        if (ct_.compare_exchange_weak(cur_ct, nxt_ct, std::memory_order_acq_rel))
        {
            break;
        }
        ipc::yield(k);
    }
    auto *el = elems + static_cast<uint8_t>(cur_ct);
    std::forward<F>(f)(&(el->data_));
    // set flag & try update wt
    el->c_.store(~static_cast<uint64_t>(cur_ct), std::memory_order_release);
    while (1)
    {
        auto cac_ct = el->c_.load(std::memory_order_acquire);
        if (cur_ct != w_.load(std::memory_order_relaxed))
        {
            return true;
        }
        if ((~cac_ct) != cur_ct)
        {
            return true;
        }
        if (!el->c_.compare_exchange_strong(cac_ct, 0, std::memory_order_relaxed))
        {
            return true;
        }
        w_.store(nxt_ct, std::memory_order_release);
        cur_ct = nxt_ct;
        nxt_ct = cur_ct + 1;
        el = elems + static_cast<uint8_t>(cur_ct);
    }
    return true;
}

template <typename W, typename F, typename R,
            template <std::size_t, std::size_t> class E, std::size_t DS, std::size_t AS>
bool pop(W * /*wrapper*/, uint32_t & /*cur*/, F &&f, R &&out, E<DS, AS> *elems)
{
    uint8_t buff[DS];
    for (unsigned k = 0;;)
    {
        auto cur_rd = r_.load(std::memory_order_relaxed);
        auto cur_wt = w_.load(std::memory_order_acquire);
        auto id_rd = static_cast<uint8_t>(cur_rd);
        auto id_wt = static_cast<uint8_t>(cur_wt);
        if (id_rd == id_wt)
        {
            auto *el = elems + id_wt;
            auto cac_ct = el->c_.load(std::memory_order_acquire);
            if ((~cac_ct) != cur_wt)
            {
                return false; // empty
            }
            if (el->c_.compare_exchange_weak(cac_ct, 0, std::memory_order_relaxed))
            {
                w_.store(cur_wt + 1, std::memory_order_release);
            }
            k = 0;
        }
        else
        {
            std::memcpy(buff, &(elems[static_cast<uint8_t>(cur_rd)].data_), sizeof(buff));
            if (r_.compare_exchange_weak(cur_rd, cur_rd + 1, std::memory_order_release))
            {
                std::forward<F>(f)(buff);
                std::forward<R>(out)(true);
                return true;
            }
            ipc::yield(k);
        }
    }
}

private:
    alignas(Align) std::atomic<uint32_t> ct_; // commit index
};



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template <>
class ProdCons<Wr<Relation::SINGLE, Relation::MULTI, Transmission::BROADCAST>>
{
public:
enum : uint64_t
{
    ep_mask = 0x00000000ffffffffull,
    ep_incr = 0x0000000100000000ull
};

template <std::size_t DataSize, std::size_t AlignSize>
struct elem_t
{
    std::aligned_storage_t<DataSize, AlignSize> data_{};
    std::atomic<uint64_t> cnt_{0}; // read-counter
};

uint32_t cursor() const noexcept
{
    return w_.load(std::memory_order_acquire);
}

template <typename W, typename F, typename E>
bool push(W *wrapper, F &&f, E *elems)
{
    E *el;
    for (unsigned k = 0;;)
    {
        uint32_t cc = wrapper->elems()->connections(std::memory_order_relaxed);
        if (cc == 0)
            return false; // no reader
        el = elems + static_cast<uint8_t>(w_.load(std::memory_order_relaxed));
        // check all consumers have finished reading this element
        auto cur_rc = el->cnt_.load(std::memory_order_acquire);
        uint32_t rem_cc = cur_rc & ep_mask;
        if ((cc & rem_cc) && ((cur_rc & ~ep_mask) == epoch_))
        {
            return false; // has not finished yet
        }
        // consider rem_cc to be 0 here
        if (el->cnt_.compare_exchange_weak(
                cur_rc, epoch_ | static_cast<uint64_t>(cc), std::memory_order_release))
        {
            break;
        }
        ipc::yield(k);
    }
    std::forward<F>(f)(&(el->data_));
    w_.fetch_add(1, std::memory_order_release);
    return true;
}

template <typename W, typename F, typename R, typename E>
bool pop(W *wrapper, uint32_t &cur, F &&f, R &&out, E *elems)
{
    if (cur == cursor())
    {
        return false; // acquire
    }
    auto *el = elems + static_cast<uint8_t>(cur++);
    std::forward<F>(f)(&(el->data_));
    for (unsigned k = 0;;)
    {
        auto cur_rc = el->cnt_.load(std::memory_order_acquire);
        if ((cur_rc & ep_mask) == 0)
        {
            std::forward<R>(out)(true);
            return true;
        }
        auto nxt_rc = cur_rc & ~static_cast<uint64_t>(wrapper->connected_id());
        if (el->cnt_.compare_exchange_weak(cur_rc, nxt_rc, std::memory_order_release))
        {
            std::forward<R>(out)((nxt_rc & ep_mask) == 0);
            return true;
        }
        ipc::yield(k);
    }
}

private:
    alignas(Align) std::atomic<uint32_t> w_; // write index
    alignas(Align) uint64_t epoch_{0};        // only one writer
};

template <>
class ProdCons<Wr<Relation::MULTI, Relation::MULTI, Transmission::BROADCAST>>
{
public:
enum : uint64_t
{
    rc_mask = 0x00000000ffffffffull,
    ep_mask = 0x00ffffffffffffffull,
    ep_incr = 0x0100000000000000ull,
    ic_mask = 0xff000000ffffffffull,
    ic_incr = 0x0000000100000000ull
};

template <std::size_t DataSize, std::size_t AlignSize>
struct elem_t
{
    std::aligned_storage_t<DataSize, AlignSize> data_{};
    std::atomic<uint64_t> cnt_{0};   // read-counter
    std::atomic<uint64_t> c_{0}; // commit flag
};

uint32_t cursor() const noexcept
{
    return ct_.load(std::memory_order_acquire);
}

constexpr static uint64_t inc_rc(uint64_t rc) noexcept
{
    return (rc & ic_mask) | ((rc + ic_incr) & ~ic_mask);
}

constexpr static uint64_t inc_mask(uint64_t rc) noexcept
{
    return inc_rc(rc) & ~rc_mask;
}

template <typename W, typename F, typename E>
bool push(W *wrapper, F &&f, E *elems)
{
    E *el;
    uint32_t cur_ct;
    uint64_t epoch = epoch_.load(std::memory_order_acquire);
    for (unsigned k = 0;;)
    {
        uint32_t cc = wrapper->elems()->connections(std::memory_order_relaxed);
        if (cc == 0)
            return false; // no reader
        el = elems + static_cast<uint8_t>(cur_ct = ct_.load(std::memory_order_relaxed));
        // check all consumers have finished reading this element
        auto cur_rc = el->cnt_.load(std::memory_order_relaxed);
        uint32_t rem_cc = cur_rc & rc_mask;
        if ((cc & rem_cc) && ((cur_rc & ~ep_mask) == epoch))
        {
            return false; // has not finished yet
        }
        else if (!rem_cc)
        {
            auto cur_fl = el->c_.load(std::memory_order_acquire);
            if ((cur_fl != cur_ct) && cur_fl)
            {
                return false; // full
            }
        }
        // consider rem_cc to be 0 here
        if (el->cnt_.compare_exchange_weak(
                cur_rc, inc_mask(epoch | (cur_rc & ep_mask)) | static_cast<uint64_t>(cc), std::memory_order_relaxed) &&
            epoch_.compare_exchange_weak(epoch, epoch, std::memory_order_acq_rel))
        {
            break;
        }
        ipc::yield(k);
    }
    // only one thread/process would touch here at one time
    ct_.store(cur_ct + 1, std::memory_order_release);
    std::forward<F>(f)(&(el->data_));
    // set flag & try update wt
    el->c_.store(~static_cast<uint64_t>(cur_ct), std::memory_order_release);
    return true;
}

template <typename W, typename F, typename R, typename E, std::size_t N>
bool pop(W *wrapper, uint32_t &cur, F &&f, R &&out, E (&elems)[N])
{
    auto *el = elems + static_cast<uint8_t>(cur);
    auto cur_fl = el->c_.load(std::memory_order_acquire);
    if (cur_fl != ~static_cast<uint64_t>(cur))
    {
        return false; // empty
    }
    ++cur;
    std::forward<F>(f)(&(el->data_));
    for (unsigned k = 0;;)
    {
        auto cur_rc = el->cnt_.load(std::memory_order_acquire);
        if ((cur_rc & rc_mask) == 0)
        {
            std::forward<R>(out)(true);
            el->c_.store(cur + N - 1, std::memory_order_release);
            return true;
        }
        auto nxt_rc = inc_rc(cur_rc) & ~static_cast<uint64_t>(wrapper->connected_id());
        bool last_one = false;
        if ((last_one = (nxt_rc & rc_mask) == 0))
        {
            el->c_.store(cur + N - 1, std::memory_order_release);
        }
        if (el->cnt_.compare_exchange_weak(cur_rc, nxt_rc, std::memory_order_release))
        {
            std::forward<R>(out)(last_one);
            return true;
        }
        ipc::yield(k);
    }
}

private:
    alignas(Align) std::atomic<uint32_t> ct_; // commit index
    alignas(Align) std::atomic<uint64_t> epoch_{0};
};

} // namespace ipc


#endif // ! _IPC_PRODCONS_H_