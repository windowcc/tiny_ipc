#ifndef _IPC_POLICY_H_
#define _IPC_POLICY_H_

#include <type_traits>
#include <ProdCons.hpp>
#include <circ/Segment.hpp>

namespace ipc
{
namespace detail
{

    template <template <typename, std::size_t...> class Elems, typename Wr>
    struct Choose;

    template <typename Wr>
    struct Choose<Segment, Wr>
    {
        template <std::size_t DataSize, std::size_t AlignSize>
        using elems_t = Segment<ipc::ProdCons<Wr>, DataSize, AlignSize>;
    };

} // namespace detail
} // namespace ipc

#endif // ! _IPC_POLICY_H_