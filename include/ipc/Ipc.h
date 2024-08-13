#ifndef _IPC_IPC_H_
#define _IPC_IPC_H_

#include <string>
#include <memory>
#include <functional>
#include <ipc/export.h>
#include <ipc/def.h>
#include <ipc/Buffer.h>
#include <ipc/Callback.h>

namespace ipc
{

template <typename Wr>
class Ipc
{
public:
    Ipc() noexcept = default;
    explicit Ipc(char const * name, const unsigned &mode = SENDER);
    Ipc(Ipc&& rhs) noexcept;

    ~Ipc();

    Ipc& operator=(Ipc rhs) noexcept;
public:
    char const * name() const noexcept;

    bool valid() const noexcept;

    unsigned mode() const noexcept;

    bool connect(char const * name, const unsigned &mode = SENDER);

    bool reconnect(unsigned mode);

    void disconnect();

    void set_callback(CallbackPtr);

    bool is_connected() const noexcept;

    bool write(void const * data, std::size_t size);

    bool write(Buffer const & buff);

    bool write(std::string const & str);

    void read(std::uint64_t tm = static_cast<uint64_t>(TimeOut::INVALID_TIMEOUT));

private:
    struct IpcImpl;
    std::unique_ptr<IpcImpl> impl_;
};


/**
 * \class route
 *
 * \note You could use one producer/server/sender for sending messages to a route,
 *       then all the consumers/clients/receivers which are receiving with this route,
 *       would receive your sent messages.
 *       A route could only be used in 1 to N (one producer/writer to multi consumers/readers).
*/
// using Route = Ipc<Wr<Relation::SINGLE, Relation::MULTI, Transmission::BROADCAST>>;

/**
 * \class channel
 *
 * \note You could use multi producers/writers for sending messages to a channel,
 *       then all the consumers/readers which are receiving with this channel,
 *       would receive your sent messages.
*/
using Channel = Ipc<Wr<Relation::MULTI, Relation::SINGLE, Transmission::UNICAST>>;

} // namespace ipc


#endif // ! _IPC_IPC_H_