#ifndef _IPC_QUEUE_H_
#define _IPC_QUEUE_H_

#include <type_traits>
#include <tuple>
#include <cassert>
#include <Handle.h>
#include <sync/RwLock.h>
#include <Resource.hpp>

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

private:
    uint32_t connected_ = 0;
    Handle handle_;
};

template <typename Segment>
class QueueBase : public QueueConn
{
    using base_t = QueueConn;
public:
    using elems_t = Segment;
    using base_t::base_t;

    QueueBase() = default;

    explicit QueueBase(char const *name)
        : QueueBase{}
    {
        elems_ = QueueConn::template open<elems_t>(name);
    }

    explicit QueueBase(elems_t *elems) noexcept
        : QueueBase{}
    {
        assert(elems != nullptr);
        elems_ = elems;
    }

    ~QueueBase()
    {
        base_t::close();
    }

    bool open(char const *name) noexcept
    {
        base_t::close();
        elems_ = QueueConn::template open<elems_t>(name);
        return elems_ != nullptr;
    }

    elems_t *elems() noexcept
    { 
        return elems_;
    }

    elems_t const *elems() const noexcept
    {
        return elems_;
    }

    bool ready_sending() noexcept
    {
        if (elems_ == nullptr)
            return false;
        return sender_flag_ || (sender_flag_ = elems_->connect_sender());
    }

    void shut_sending() noexcept
    {
        if (elems_ == nullptr)
            return;
        if (!sender_flag_)
            return;
        elems_->disconnect_sender();
    }

    bool connect() noexcept
    {
        auto tp = base_t::connect(elems_);
        if (std::get<0>(tp) && std::get<1>(tp))
        {
            cursor_ = std::get<2>(tp);
            return true;
        }
        return std::get<0>(tp);
    }

    bool disconnect() noexcept
    {
        return base_t::disconnect(elems_);
    }

    std::size_t conn_count() const noexcept
    {
        return (elems_ == nullptr) ? static_cast<std::size_t>(TimeOut::DEFAULT_TIMEOUT) : elems_->conn_count();
    }

    bool valid() const noexcept
    {
        return elems_ != nullptr;
    }

    bool empty() const noexcept
    {
        return !valid() || (cursor_ == elems_->cursor());
    }

    template <typename Msg, typename... P>
    bool push(P &&...params)
    {
        if (elems_ == nullptr)
        {
            return false;
        }
        return elems_->push(this, [&](void *p)
        {
            ::new (p) Msg(std::forward<P>(params)...);
        });
    }

    template <typename Msg, typename F>
    bool pop(Msg &item, F &&out)
    {
        if (elems_ == nullptr)
        {
            return false;
        }
        return elems_->pop(this, &(this->cursor_), [&item](void *p)
        {
            ::new (&item) Msg(std::move(*static_cast<Msg *>(p)));
        }, std::forward<F>(out));
    }

public:
    elems_t *elems_ = nullptr;
    decltype(std::declval<elems_t>().cursor()) cursor_ = 0;
    bool sender_flag_ = false;
};

} // namespace detail

template <typename Msg, typename Choose>
class Queue final : public detail::QueueBase<typename Choose::template elems_t<sizeof(Msg), alignof(Msg)>>
{
    using base_t = detail::QueueBase<typename Choose::template elems_t<sizeof(Msg), alignof(Msg)>>;

public:
    using base_t::base_t;
public:
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