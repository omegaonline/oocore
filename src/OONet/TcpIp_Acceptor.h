#pragma once

#include "../OOSvc/Transport_Acceptor.h"
#include "../OOsvc/Shutdown.h"

#include "./TcpIp_Svc_Handler.h"

class OONet_TcpIp_Acceptor : 
	public OONet_TcpIp_Svc_Handler<OOSvc_Transport_Acceptor>,
	public OOSvc_Shutdown_Observer
{
	typedef OONet_TcpIp_Svc_Handler<OOSvc_Transport_Acceptor> svc_base;

private:
	void handle_shutdown()
	{
		svc_base::close();
	}
};
