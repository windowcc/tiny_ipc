#ifndef _IPC_CORE_ELEMARRAY_H_
#define _IPC_CORE_ELEMARRAY_H_

#include <atomic>
#include <limits>
#include <utility>
#include <type_traits>
#include <core/Head.hpp>

namespace ipc
{
namespace detail
{

template <typename Policies,
          std::size_t DataSize,
          std::size_t AlignSize = (std::min)(DataSize, alignof(std::max_align_t))>
class Segment : public ipc::detail::Head<Policies>
{
public:
    using base_t   = ipc::detail::Head<Policies>;
    using cursor_t = decltype(std::declval<Policies>().cursor());
    using elem_t   = typename Policies::template elem_t<DataSize, AlignSize>;

public:
    bool connect_sender() noexcept
    {
        return c_s_.connect();
    }

    void disconnect_sender() noexcept
    {
        return c_s_.disconnect();
    }

    uint32_t connect_receiver() noexcept
    {
        return c_r_.connect(*this);
    }

    uint32_t disconnect_receiver(uint32_t cc_id) noexcept
    {
        return c_r_.disconnect(*this, cc_id);
    }

    cursor_t cursor() const noexcept
    {
        return policies_.cursor();
    }

    template <typename Q, typename F>
    bool push(Q* que, F&& f)
    {
        return policies_.push(que, std::forward<F>(f), block_);
    }

    template <typename Q, typename F, typename R>
    bool pop(Q* que, cursor_t* cur, F&& f, R&& out)
    {
        if (cur == nullptr)
        {
            return false;
        }
        return policies_.pop(que, *cur, std::forward<F>(f), std::forward<R>(out), block_);
    }

private:
    template <typename P, bool/* = RelationTrait<P>::is_multi_producer*/>
    struct sender_checker;
    template <typename P>
    struct sender_checker<P, true>
    {
        constexpr static bool connect() noexcept
        {
            // always return true
            return true;
        }
        constexpr static void disconnect() noexcept {}
    };

    template <typename P>
    struct sender_checker<P, false>
    {
        bool connect() noexcept
        {
            return !flag_.test_and_set(std::memory_order_acq_rel);
        }
        void disconnect() noexcept
        {
            flag_.clear();
        }

    private:
        // in shm, it should be 0 whether it's initialized or not.
        std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
    };

    template <typename P, bool/* = RelationTrait<P>::is_multi_consumer*/>
    struct receiver_checker;

    template <typename P>
    struct receiver_checker<P, true>
    {
        constexpr static uint32_t connect(base_t &conn) noexcept
        {
            return conn.connect();
        }
        constexpr static uint32_t disconnect(base_t &conn, uint32_t cc_id) noexcept
        {
            return conn.disconnect(cc_id);
        }
    };

    template <typename P>
    struct receiver_checker<P, false> : protected sender_checker<P, false>
    {
        uint32_t connect(base_t &conn) noexcept
        {
            return sender_checker<P, false>::connect() ? conn.connect() : 0;
        }

        uint32_t disconnect(base_t &conn, uint32_t cc_id) noexcept
        {
            sender_checker<P, false>::disconnect();
            return conn.disconnect(cc_id);
        }
    };

private:
    Policies policies_;
    elem_t   block_[(std::numeric_limits<uint8_t>::max)() + 1] {};
    sender_checker<Policies, RelationTrait<Policies>::is_multi_producer> c_s_;
    receiver_checker<Policies, RelationTrait<Policies>::is_multi_consumer> c_r_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_CORE_ELEMARRAY_H_