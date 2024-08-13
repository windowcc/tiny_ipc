#include <ipc/Ipc.h>
#include <shared_mutex>
#include <Handle.h>
#include <MessageQueue.hpp>
#include <Choose.hpp>
#include <ipc/Callback.h>
#include <core/Fragment.hpp>
#include <core/Segment.hpp>

using namespace ipc::detail;

namespace ipc
{

#define handle_impl         (impl_->handle)
#define fragment_impl       (impl_->fragment)
#define mode_impl           (impl_->mode)
#define connected_impl      (impl_->connected)
#define callback_impl       (impl_->callback)
template<typename Wr>
struct Ipc<Wr>::IpcImpl
{
    // 用于发送描述信息
    std::shared_ptr<MessageQueue<Choose<Segment, Wr>> > handle { nullptr };
    std::unique_ptr<FragmentBase> fragment {nullptr};
    unsigned mode { SENDER };
    std::atomic_bool connected { false };
    CallbackPtr callback {nullptr};
};

template <typename Wr>
Ipc<Wr>::Ipc(char const * name, const unsigned &mode)
    : impl_ {std::make_unique<Ipc<Wr>::IpcImpl>()}
{
    connect(name, mode);
}

template <typename Wr>
Ipc<Wr>::Ipc(Ipc&& rhs) noexcept
        : Ipc{}
{
    impl_.swap(rhs.impl_);
}

template <typename Wr>
Ipc<Wr>::~Ipc()
{
    disconnect();
}

template <typename Wr>
Ipc<Wr>& Ipc<Wr>::operator=(Ipc<Wr> rhs) noexcept
{
    impl_.swap(rhs.impl_);
    return *this;
}

template <typename Wr>
char const * Ipc<Wr>::name() const noexcept
{
    return (handle_impl == nullptr) ? nullptr : handle_impl->name().c_str();
}

template <typename Wr>
bool Ipc<Wr>::valid() const noexcept
{
    return (handle_impl != nullptr);
}

template <typename Wr>
unsigned Ipc<Wr>::mode() const noexcept
{
    return mode_impl;
}

template <typename Wr>
bool Ipc<Wr>::connect(char const * name, const unsigned &mode)
{
    if (name == nullptr || name[0] == '\0')
    {
        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_NOINIT);
        }
        return false;
    }

    switch (mode)
    {
    case static_cast<unsigned>(SENDER):
        fragment_impl = std::make_unique<Fragment<SENDER>>();
        break;
    case static_cast<unsigned>(RECEIVER):
        fragment_impl = std::make_unique<Fragment<RECEIVER>>();
        break;
    default:
        break;
    }

    if(!fragment_impl->init())
    {
        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_NOINIT);
        }
        return false;
    }

    disconnect();
    if(!valid())
    {
        handle_impl = std::make_shared<MessageQueue<Choose<Segment, Wr>> >(nullptr,name);
        if(handle_impl->init())
        {
            if(callback_impl)
            {
                callback_impl->connected(ErrorCode::IPC_ERR_NOINIT);
            }
            return false;
        }
    }
    mode_impl = mode;
    return connected_impl = reconnect(mode);
}

template <typename Wr>
bool Ipc<Wr>::reconnect(unsigned mode)
{
    if (!valid())
    {
        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_INVAL);
        }
        return false;
    }
    if (connected_impl && (mode_impl == mode))
    {
        if(callback_impl)
        {
            callback_impl->connected();
        }
        return true;
    }

    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_NOMEM);
        }
        return false;
    }

    if(!(handle_impl->init()))
    {
        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_NOINIT);
        }
        return false;
    }

    if (mode & RECEIVER)
    {
        que->shut_sending();
        if (que->connect())
        {
            if(callback_impl)
            {
                callback_impl->connected();
            }
            return true;
        }

        if(callback_impl)
        {
            callback_impl->connected(ErrorCode::IPC_ERR_NOINIT);
        }
        return false;
    }

    if (que->connected())
    {
        handle_impl->disconnect();
    }

    if(callback_impl)
    {
        callback_impl->connected();
    }

    return que->ready_sending();
}

template <typename Wr>
void Ipc<Wr>::disconnect()
{
    if (!valid())
    {
        if(callback_impl)
        {
            callback_impl->connection_lost(ErrorCode::IPC_ERR_NOMEM);
        }
        return;
    }
    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        if(callback_impl)
        {
            callback_impl->connection_lost(ErrorCode::IPC_ERR_NOMEM);
        }
        return;
    }
    connected_impl = false;
    que->shut_sending();
    assert((handle_impl) != nullptr);
    handle_impl->disconnect();

    if(callback_impl)
    {
        callback_impl->connection_lost();
    }
}

template <typename Wr>
void Ipc<Wr>::set_callback(CallbackPtr callback)
{
    if(!callback_impl)
    {
        callback_impl = callback;
    }
}

template <typename Wr>
bool Ipc<Wr>::is_connected() const noexcept
{
    return connected_impl;
}

template <typename Wr>
bool Ipc<Wr>::write(void const *data, std::size_t size)
{
    if (!valid() || data == nullptr || size == 0)
    {
        if(callback_impl)
        {
            callback_impl->delivery_complete(ErrorCode::IPC_ERR_NOINIT);
        }
        return false;
    }
    auto que = handle_impl->queue();
    if (que == nullptr || que->elems() == nullptr || !que->ready_sending() ||
            que->elems()->connections(std::memory_order_relaxed) == 0)
    {
        if(callback_impl)
        {
            callback_impl->delivery_complete(ErrorCode::IPC_ERR_NOMEM);
        }
        return false;
    }

    auto desc = fragment_impl->write(data,size);
    if(!desc.length() || !que->push(desc))
    {
        if(callback_impl)
        {
            callback_impl->delivery_complete(ErrorCode::IPC_ERR_INVAL);
        }
        return false;
    }

    if(Wr::is_broadcast)
    {
        handle_impl->waiter()->broadcast();
    }
    else
    {
        handle_impl->waiter()->notify();
    }
    

    if(callback_impl)
    {
        callback_impl->delivery_complete();
    }

    return true;
}

template <typename Wr>
bool Ipc<Wr>::write(Buffer const & buff)
{
    return this->write(buff.data(), buff.size());
}

template <typename Wr>
bool Ipc<Wr>::write(std::string const & str)
{
    return this->write(str.c_str(), str.size());
}

static std::string thread_id_to_string(const std::thread::id &id)
{
    std::ostringstream oss;
    oss << id;
    return oss.str();
}

template <typename Wr>
void Ipc<Wr>::read(std::uint64_t tm)
{
    if(!valid())
    {
        if(callback_impl)
        {
            callback_impl->message_arrived(ErrorCode::IPC_ERR_NOMEM);
        }
        return ;
    }
    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        if(callback_impl)
        {
            callback_impl->message_arrived(ErrorCode::IPC_ERR_NOMEM);
        }
        return ;
    }
    if (!que->connected())
    {
        if(callback_impl)
        {
            callback_impl->message_arrived(ErrorCode::IPC_ERR_NO_CONN);
        }
        return ;
    }
    for (;;)
    {
        if(!is_connected())
        {
            break;
        }

        handle_impl->wait_for([&]
        {
            Descriptor desc{};
            while(!que->empty())
            {
                if(!que->pop(desc))
                {
                    return;
                }
                auto buf = fragment_impl->read(desc);
                if(!buf.empty())
                {
                    callback_impl->message_arrived(&buf);
                }
            }
        }, tm);
    }
}

// UNICAST 一个通道对应一个Read
template struct Ipc<Wr<Relation::SINGLE, Relation::SINGLE, Transmission::UNICAST>>;
template struct Ipc<Wr<Relation::MULTI, Relation::SINGLE, Transmission::UNICAST>>;

// BROADCAST 一个通道对应多个Read
template struct Ipc<Wr<Relation::SINGLE, Relation::MULTI, Transmission::BROADCAST>>;
template struct Ipc<Wr<Relation::MULTI, Relation::MULTI, Transmission::BROADCAST>>;


} // namespace ipc
