#pragma once

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>
#include <ace/SOCK_Stream.h>

#include "../OOCore/Object.h"
#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Manager.h"
#include "../OOSvc/Transport_Connector.h"

class OONet_TcpIp_Connector :
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Connector,ACE_SOCK_STREAM,2048>
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Connector,ACE_SOCK_STREAM,2048> svc_base;

public:
};


