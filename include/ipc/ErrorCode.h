#ifndef _IPC_ERRORCODE_H_
#define _IPC_ERRORCODE_H_

#include <stdint.h>

namespace ipc {

enum class ErrorCode : uint8_t
{
	IPC_ERR_SUCCESS = 0,
	IPC_ERR_NOMEM,
	IPC_ERR_INVAL,
	IPC_ERR_NO_CONN,
	IPC_ERR_CONN_REFUSED,
	IPC_ERR_NOT_FOUND,
	IPC_ERR_CONN_LOST,
	IPC_ERR_NOT_SUPPORTED,
	IPC_ERR_UNKNOWN,
};

} // namespace ipc

#endif // ! _IPC_ERRORCODE_H_