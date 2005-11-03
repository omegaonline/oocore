#ifndef _OOCORE_CONNECTION_MANAGER_H_INCLUDED_
#define _OOCORE_CONNECTION_MANAGER_H_INCLUDED_

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/MEM_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOCore/Transport_Connector.h"
#include "../OOCore/Object.h"

class OOCore_Connection_Manager :
	public OOCore_Transport_Svc_Handler<OOCore_Transport_Connector,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
	typedef OOCore_Transport_Svc_Handler<OOCore_Transport_Connector,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;

public:
	static int init(void);

	int close(u_long flags = 0)
	{
		if (OOCore_Transport_Connector::close() != 0)
			return -1;

		shutdown();

		return 0;
	}

protected:
	ssize_t send_n(ACE_Message_Block* mb)
	{
		return this->peer().send(mb,0);
	}
};

typedef ACE_Singleton<OOCore_Connection_Manager, ACE_Thread_Mutex> CONNECTION_MANAGER;

#endif // _OOCORE_CONNECTION_MANAGER_H_INCLUDED_
