#ifndef _IPC_CONNECT_INFO_H_
#define _IPC_CONNECT_INFO_H_

#include <Queue.hpp>
#include <BufferDesc.hpp>
#include <sync/Waiter.h>

namespace ipc
{
namespace detail
{

template <typename Choose>
class MessageQueue
{
public:
    using queue_t = Queue<BufferDesc, Choose>;
public:
    explicit MessageQueue(char const *prefix, char const *name)
        : prefix_{make_string(prefix)}
        , name_{make_string(name)}
        { 
            // init();
        }
    ~MessageQueue()
    {

    }
public:
    bool init()
    {
        if (!queue_.valid())
        {
            if(queue_.open(make_prefix(prefix_,
                {to_string(sizeof(BufferDesc)), "_",to_string(alignof(std::max_align_t)), "_",this->name_}).c_str()))
            {
                return false;
            }
        }
        return true;
    }

    inline std::string prefix() const
    {
        return prefix_;
    }

    inline std::string name() const
    {
        return name_;
    }

    inline Waiter *waiter()
    {
        return &(queue_.segment_->waiter_);
    }

    inline queue_t *queue()
    {
        return &queue_;
    }

    template <typename F>
    bool wait_for(F &&pred, std::uint64_t tm)
    {
        return tm == 0 ? !pred() : 
            waiter()->wait_for(std::forward<F>(pred), tm);
    }

    void disconnect()
    {
        waiter()->quit();
        queue_.disconnect();
    }
private:
    string prefix_;
    string name_;
    queue_t queue_;
};




} // namespace detail
} // namespace ipc

#endif // ! _IPC_CONNECT_INFO_H_