#ifndef _IPC_POLICY_H_
#define _IPC_POLICY_H_

#include <type_traits>
#include <core/Content.hpp>
#include <core/Segment.hpp>

namespace ipc
{
namespace detail
{

    template <template <typename, std::size_t...> class Segment, typename Wr>
    struct Choose;

    template <typename Wr>
    struct Choose<Segment, Wr>
    {
        template <std::size_t DataSize, std::size_t AlignSize>
        using segment_t = Segment<Content<Wr>, DataSize, AlignSize>;
    };

} // namespace detail
} // namespace ipc

#endif // ! _IPC_POLICY_H_