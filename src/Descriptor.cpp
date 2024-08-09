#include <Descriptor.h>

namespace ipc
{
namespace detail
{

Descriptor::Descriptor()
    :Descriptor(std::thread::id(),std::size_t(),std::size_t())
{

}

Descriptor::Descriptor(const std::thread::id &id,const std::size_t &offset, const std::size_t &len)
    : id_(id)
    , offset_(offset)
    , length_(len)
{

}

Descriptor::~Descriptor()
{

}

void Descriptor::id(const std::thread::id &id)
{
    id_ = id;
}

std::thread::id Descriptor::id() const
{
    return id_;
}

void Descriptor::offset(const std::size_t &offset)
{
    offset_ = offset;
}

std::size_t Descriptor::offset() const
{
    return offset_;
}

void Descriptor::length(const std::size_t &length)
{
    length_ = length;
}

std::size_t Descriptor::length() const
{
    return length_;
}


} // namespace detail
} // namespace ipc