#include <PoolAlloc.h>
#include <Resource.hpp>
#include <memory/Alloc.hpp>

namespace ipc
{
namespace detail
{

void *PoolAlloc::alloc(std::size_t size)
{
    return StaticAlloc::alloc(size);
}

void PoolAlloc::free(void *p, std::size_t size)
{
    StaticAlloc::free(p, size);
}

} // namespace detail
} // namespace ipc
