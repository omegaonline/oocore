//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_LOCAL_TRANSPORT_H_INCLUDED_
#define OOCORE_LOCAL_TRANSPORT_H_INCLUDED_

#include <ace/Thread_Mutex.h>
#include <ace/MEM_Stream.h>

#include "./Transport_Svc_Handler.h"

namespace OOCore
{
namespace Impl
{

class LocalTransport :
	public OOCore::Transport_Svc_Handler<ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
	typedef OOCore::Transport_Svc_Handler<ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;

	friend class ACE_DLL_Singleton<OOUtil::SingletonObject<LocalTransport>, ACE_Thread_Mutex>;

public:
	static int init(void);

protected:
	LocalTransport() {}
	virtual ~LocalTransport() {}

private:
	ACE_Thread_Mutex	m_lock;

	int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0);
	ssize_t send_n(ACE_Message_Block* mb);

	static int launch_server(ACE_CString& strExeName);
};

typedef ACE_DLL_Singleton<OOUtil::SingletonObject<LocalTransport>, ACE_Thread_Mutex> LOCAL_TRANSPORT;

};
};

#endif // OOCORE_LOCAL_TRANSPORT_H_INCLUDED_
