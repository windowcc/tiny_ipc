#ifndef _IPC_CALLBACK_H_
#define _IPC_CALLBACK_H_

#include <vector>
#include <memory>
// #include <iostream>
// #include <ipc/Buffer.h>

namespace ipc {

class Buffer;

/////////////////////////////////////////////////////////////////////////////

/**
 * Provides a mechanism for tracking the completion of an asynchronous
 * action.
 */
class IPC_EXPORT Callback
{
public:
    /** Smart/shared pointer to an object of this type */
	using ptr_t = std::shared_ptr<Callback>;
	/** Smart/shared pointer to a const object of this type */
	using const_ptr_t = std::shared_ptr<const Callback>;

	virtual ~Callback() {}

	// virtual void connected(const string& /*cause*/) {}
	// virtual void connection_lost(const string& /*cause*/) {}
	virtual void message_arrived(const Buffer *buf /*msg*/) {}
	// virtual void delivery_complete(delivery_token_ptr /*tok*/) {}
};

/** Smart/shared pointer to a callback object */
using CallbackPtr = Callback::ptr_t;

/** Smart/shared pointer to a const callback object */
using ConstCallbackPtr = Callback::const_ptr_t;

} // namespace ipc

#endif // _IPC_CALLBACK_H_