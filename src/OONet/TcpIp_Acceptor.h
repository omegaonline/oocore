#pragma once

#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Acceptor.h"
#include "../OOsvc/Shutdown.h"

class OONet_TcpIp_Acceptor : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_SOCK_STREAM,2048>,
	public OOSvc_Shutdown_Observer
{
public:
	int open(void* p);

private:
	void handle_shutdown();
};
