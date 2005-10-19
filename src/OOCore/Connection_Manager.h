#pragma once

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
};

typedef ACE_Singleton<OOCore_Connection_Manager, ACE_Thread_Mutex> CONNECTION_MANAGER;
