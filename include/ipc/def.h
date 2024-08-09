#ifndef _IPC_DEF_H_
#define _IPC_DEF_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <utility>

namespace ipc {

// constants

enum : unsigned
{
    SENDER = 1,
    RECEIVER
};

enum class TimeOut : std::uint64_t
{
    DEFAULT_TIMEOUT = 100, // ms
    INVALID_TIMEOUT = (std::numeric_limits<std::uint64_t>::max)(),
};

enum class Relation : uint32_t
{
    SINGLE,
    MULTI
};

enum class Transmission : uint32_t
{
    UNICAST,
    BROADCAST
};

// producer-consumer policy flag
template <Relation Rp, Relation Rc, Transmission Ts>
struct Wr
{
    constexpr static bool is_broadcast = (Ts == Transmission::BROADCAST);
};

} // namespace ipc

#endif // ! _IPC_DEF_H_