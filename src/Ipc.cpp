#include <ipc/Ipc.h>
#include <shared_mutex>
#include <Handle.h>
#include <PoolAlloc.h>
#include <MessageQueue.hpp>
#include <Choose.hpp>
#include <fragment/Fragment.hpp>
#include <circ/Segment.hpp>

using namespace ipc::detail;

namespace ipc
{

#define handle_impl         (impl_->handle)
#define fragment_impl       (impl_->fragment)
#define mode_impl           (impl_->mode)
#define connected_impl      (impl_->connected)
template<typename Wr>
struct Ipc<Wr>::IpcImpl
{
    // 用于发送描述信息
    std::shared_ptr<MessageQueue<Choose<Segment, Wr>> > handle { nullptr };
    std::shared_ptr<FragmentBase> fragment {nullptr};
    unsigned mode { SENDER };
    bool connected { false };
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
        return false;
    }
    switch (mode)
    {
    case static_cast<unsigned>(SENDER):
        {
            std::shared_ptr<FragmentBase> tmp(new Fragment<SENDER>(),[](FragmentBase *ptr){
                delete (static_cast<Fragment<SENDER>*>(ptr));
            });
            fragment_impl.swap(tmp);
        }
        break;
    case static_cast<unsigned>(RECEIVER):
        {
            std::shared_ptr<FragmentBase> tmp(new Fragment<RECEIVER>(),[](FragmentBase *ptr){
                    delete (static_cast<Fragment<RECEIVER>*>(ptr));
                });
            fragment_impl.swap(tmp);
        }
        break;
    default:
        break;
    }
    if(!fragment_impl->init())
    {
        return false;
    }

    disconnect();
    if(!valid())
    {
        handle_impl = std::make_shared<MessageQueue<Choose<Segment, Wr>> >(nullptr,name);
        if(handle_impl->init())
        {
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
        return false;
    }
    if (connected_impl && (mode_impl == mode))
    {
        return true;
    }

    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        return false;
    }

    handle_impl->init();

    if (mode & RECEIVER)
    {
        que->shut_sending();
        if (que->connect())
        {
            return true;
        }
        return false;
    }

    if (que->connected())
    {
        handle_impl->disconnect();
    }
    return que->ready_sending();
}

template <typename Wr>
void Ipc<Wr>::disconnect()
{
    if (!valid())
    {
        return;
    }

    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        return;
    }
    que->shut_sending();
    assert((handle_impl) != nullptr);
    handle_impl->disconnect();
    connected_impl = false;
}

template <typename Wr>
bool Ipc<Wr>::write(void const *data, std::size_t size)
{
    if (!valid() || data == nullptr || size == 0)
    {
        return false;
    }
    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        return false;
    }
    if (que->elems() == nullptr)
    {
        return false;
    }
    if (!que->ready_sending())
    {
        return false;
    }
    uint32_t conns = que->elems()->connections(std::memory_order_relaxed);
    if (conns == 0)
    {
        return false;
    }

    auto desc = fragment_impl->write(data,size);
    if(!desc.valid())
    {
        return false;
    }

    if(!que->push(desc))
    {
        return false;
    }
    handle_impl->waiter()->broadcast();
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
Buffer Ipc<Wr>::read(std::uint64_t tm)
{
    if(!valid())
    {
        return {};
    }
    auto que = handle_impl->queue();
    if (que == nullptr)
    {
        return {};
    }
    if (!que->connected())
    {
        return {};
    }
    auto inf = handle_impl;

    for (;;)
    {
        // pop a new message
        BufferDesc desc{};
        if (!inf->wait_for([que, &desc]
        {
            return !que->pop(desc);
        }, tm))
        {
            return {};
        }
        return std::move(fragment_impl->read(desc));
    }
}

// UNICAST 一个通道对应一个Read
template struct Ipc<Wr<Relation::SINGLE, Relation::SINGLE, Transmission::UNICAST>>;
template struct Ipc<Wr<Relation::MULTI, Relation::SINGLE, Transmission::UNICAST>>;

// BROADCAST 一个通道对应多个Read
template struct Ipc<Wr<Relation::SINGLE, Relation::MULTI, Transmission::BROADCAST>>;
template struct Ipc<Wr<Relation::MULTI, Relation::MULTI, Transmission::BROADCAST>>;


} // namespace ipc
