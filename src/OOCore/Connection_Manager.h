#pragma once

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "../OOCore/Client_Svc_Handler.h"
#include "../OOCore/Transport_Connector.h"
#include "../OOCore/Object.h"

class OOCore_Connection_Manager :
	public OOCore_Client_Svc_Handler<OOCore_Transport_Connector>
{
	typedef OOCore_Client_Svc_Handler<OOCore_Transport_Connector> svc_base;

public:
	static int init(void);
};

typedef ACE_Singleton<OOCore_Connection_Manager, ACE_Thread_Mutex> CONNECTION_MANAGER;
