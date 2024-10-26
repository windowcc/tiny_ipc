#ifndef _IPC_MESSAGE_H_
#define _IPC_MESSAGE_H_

#include <ipc/def.h>
#include <thread>

namespace ipc
{
namespace detail
{

struct alignas(alignof(std::max_align_t)) BufferDesc
{

    inline bool valid() const
    {
        return len != 0;
    }

    std::thread::id id = {};     // 该属性用于计算buffer所对应的内存空间,名称通过 to_string(id)计算
    std::size_t offset = {0};        // buffer 对应的缓存的position
    std::size_t len = {0};           // buffer 长度
};

} // namespace detail
} // namespace ipc

#endif // ! _IPC_MESSAGE_H_