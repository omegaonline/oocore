//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_CONNECTION_MANAGER_H_INCLUDED_
#define OOCORE_CONNECTION_MANAGER_H_INCLUDED_

#include <ace/Thread_Mutex.h>
#include <ace/MEM_Stream.h>

#include "./Transport_Svc_Handler.h"

namespace OOCore
{
namespace Impl
{

class Connection_Manager :
	public OOCore::Transport_Svc_Handler<ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
	typedef OOCore::Transport_Svc_Handler<ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;

public:
	static int init(void);

private:
	friend class ACE_Singleton<Connection_Manager, ACE_Thread_Mutex>;
	virtual ~Connection_Manager() {}

	ACE_Thread_Mutex	m_lock;

	int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0);
	ssize_t send_n(ACE_Message_Block* mb);

	static bool await_connect(void * p);
};

typedef ACE_Singleton<Connection_Manager, ACE_Thread_Mutex> CONNECTION_MANAGER;

};
};

#endif // OOCORE_CONNECTION_MANAGER_H_INCLUDED_
