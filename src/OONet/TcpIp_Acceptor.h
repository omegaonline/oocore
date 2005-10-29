#ifndef _OONET_TCPIP_ACCEPTOR_H_INCLUDED_
#define _OONET_TCPIP_ACCEPTOR_H_INCLUDED_

#include <ace/Acceptor.h>
#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Acceptor.h"
#include "../OOSvc/Shutdown.h"

class OONet_TcpIp_Manager;

class OONet_TcpIp_Acceptor : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_SOCK_STREAM,2048>,
	public OOSvc_Shutdown_Observer
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_SOCK_STREAM,2048> acceptor_base;

public:
	OONet_TcpIp_Acceptor();

	int open(void* p);

private:
	int on_close();
	void handle_shutdown();

	OONet_TcpIp_Manager* m_protocol;
};

#endif // _OONET_TCPIP_ACCEPTOR_H_INCLUDED_
