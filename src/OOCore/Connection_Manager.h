//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_CONNECTION_MANAGER_H_INCLUDED_
#define OOCORE_CONNECTION_MANAGER_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/MEM_Stream.h>

#include "./Transport_Svc_Handler.h"

namespace OOCore
{
namespace Impl
{

class Connection_Manager :
	public OOCore::Transport_Svc_Handler<false,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
	typedef OOCore::Transport_Svc_Handler<false,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;

public:
	static int init(void);

private:
	virtual ~Connection_Manager() {}

	friend class ACE_Singleton<Connection_Manager, ACE_Thread_Mutex>;

	ssize_t send_n(ACE_Message_Block* mb);
};

typedef ACE_Singleton<Connection_Manager, ACE_Thread_Mutex> CONNECTION_MANAGER;

};
};

#endif // OOCORE_CONNECTION_MANAGER_H_INCLUDED_
