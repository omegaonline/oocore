#pragma once

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Acceptor.h"
#include "../OOsvc/Shutdown.h"

class OONet_TcpIp_Acceptor : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_SOCK_STREAM,2048>,
	public OOSvc_Shutdown_Observer
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_SOCK_STREAM,2048> svc_base;

private:
	void handle_shutdown()
	{
		svc_base::close();
	}
};
