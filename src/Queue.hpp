#ifndef _IPC_QUEUE_H_
#define _IPC_QUEUE_H_

#include <type_traits>
#include <tuple>
#include <cassert>
#include <Handle.h>
#include <sync/RwLock.h>
#include <Resource.hpp>
#include <iostream>

namespace ipc
{
namespace detail
{

class QueueConn
{
public:
    QueueConn() = default;
    QueueConn(const QueueConn &) = delete;
    QueueConn &operator=(const QueueConn &) = delete;

    bool connected() const noexcept
    {
        return connected_ != 0;
    }

    uint32_t connected_id() const noexcept
    {
        return connected_;
    }

    template <typename Segment>
    auto connect(Segment *elems) noexcept
        -> std::tuple<bool, bool, decltype(std::declval<Segment>().cursor())>
    {
        if (elems == nullptr)
            return {};
        // if it's already connected, just return
        if (connected())
            return {connected(), false, 0};
        connected_ = elems->connect_receiver();
        return {connected(), true, elems->cursor()};
    }

    template <typename Segment>
    bool disconnect(Segment *elems) noexcept
    {
        if (elems == nullptr)
            return false;
        if (!connected())
            return false;
        elems->disconnect_receiver(std::exchange(connected_, 0));
        return true;
    }

protected:
    template <typename Segment>
    Segment *open(char const *name)
    {
        if (!is_valid_string(name))
        {
            return nullptr;
        }
        if (!handle_.acquire(name, sizeof(Segment)))
        {
            return nullptr;
        }
        auto elems = static_cast<Segment *>(handle_.get());
        if (elems == nullptr)
        {
            return nullptr;
        }
        elems->init();
        return elems;
    }

    void close()
    {
        handle_.release();
    }

protected:
    uint32_t connected_ = 0;
    Handle handle_;
};

template <typename Segment>
class QueueBase : public QueueConn
{
    using base_t = QueueConn;
public:
    using segment_t = Segment;
    using base_t::base_t;

    QueueBase() = default;

    explicit QueueBase(char const *name)
        : QueueBase{}
    {
        segment_ = QueueConn::template open<segment_t>(name);
    }

    explicit QueueBase(segment_t *segment) noexcept
        : QueueBase{}
    {
        assert(segment != nullptr);
        segment_ = segment;
    }

    virtual ~QueueBase()
    {

        if(handle_.ref() <= 1)
        {
            segment_->waiter_.close();
        }
        base_t::close();
    }

    bool open(char const *name) noexcept
    {
        base_t::close();
        segment_ = QueueConn::template open<segment_t>(name);
        return segment_ != nullptr;
    }

    segment_t *elems() noexcept
    { 
        return segment_;
    }

    segment_t const *elems() const noexcept
    {
        return segment_;
    }

    bool ready_sending() noexcept
    {
        if (segment_ == nullptr)
            return false;
        return sender_flag_ || (sender_flag_ = segment_->connect_sender());
    }

    void shut_sending() noexcept
    {
        if (segment_ == nullptr)
            return;
        if (!sender_flag_)
            return;
        segment_->disconnect_sender();
    }

    bool connect() noexcept
    {
        auto tp = base_t::connect(segment_);
        if (std::get<0>(tp) && std::get<1>(tp))
        {
            cursor_ = std::get<2>(tp);
            return true;
        }
        return std::get<0>(tp);
    }

    bool disconnect() noexcept
    {
        return base_t::disconnect(segment_);
    }

    std::size_t conn_count() const noexcept
    {
        return (segment_ == nullptr) ? static_cast<std::size_t>(TimeOut::DEFAULT_TIMEOUT) : segment_->conn_count();
    }

    bool valid() const noexcept
    {
        return segment_ != nullptr;
    }

    bool empty() const noexcept
    {
        return !valid() || (cursor_ == segment_->cursor());
    }

    template <typename Msg, typename... P>
    bool push(P &&...params)
    {
        if (segment_ == nullptr)
        {
            return false;
        }
        return segment_->push(this, [&](void *p)
        {
            ::new (p) Msg(std::forward<P>(params)...);
        });
    }

    template <typename Msg, typename F>
    bool pop(Msg &item, F &&out)
    {
        if (segment_ == nullptr)
        {
            return false;
        }
        return segment_->pop(this, &(this->cursor_), [&item](void *p)
        {
            ::new (&item) Msg(std::move(*static_cast<Msg *>(p)));
        }, std::forward<F>(out));
    }

public:
    segment_t *segment_ = nullptr;
    decltype(std::declval<segment_t>().cursor()) cursor_ = 0;
    bool sender_flag_ = false;
};

} // namespace detail

template <typename Msg, typename Choose>
class Queue final : public detail::QueueBase<typename Choose::template segment_t<sizeof(Msg), alignof(Msg)>>
{
    using base_t = detail::QueueBase<typename Choose::template segment_t<sizeof(Msg), alignof(Msg)>>;

public:
    using base_t::base_t;

    template <typename... P>
    bool push(P &&...params)
    {
        return base_t::template push<Msg>(std::forward<P>(params)...);
    }

    bool pop(Msg &item)
    {
        return base_t::pop(item, [](bool) {});
    }

    template <typename F>
    bool pop(Msg &item, F &&out)
    {
        return base_t::pop(item, std::forward<F>(out));
    }
};

} // namespace ipc


#endif // ! _IPC_QUEUE_H_