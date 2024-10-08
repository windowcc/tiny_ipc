#ifndef _IPC_MESSAGE_H_
#define _IPC_MESSAGE_H_

#include <ipc/def.h>
#include <thread>

namespace ipc
{
namespace detail
{

class Descriptor
{
public:
    Descriptor();
    Descriptor(const std::thread::id &id,const std::size_t &offset, const std::size_t &len);
    ~Descriptor();
public:
    void id(const std::thread::id &id);
    std::thread::id id() const;

    void offset(const std::size_t &offset);
    std::size_t offset() const;

    void length(const std::size_t &size);
    std::size_t length() const;

private:
    // data storage file name generate
    std::thread::id id_;
    // data offset
    std::size_t offset_;
    // data length
    std::size_t length_;
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_MESSAGE_H_