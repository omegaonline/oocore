#ifndef _OONET_TCPIP_CONNECTOR_H_INCLUDED_
#define _OONET_TCPIP_CONNECTOR_H_INCLUDED_

#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Connector.h"

class OONet_TcpIp_Manager;

class OONet_TcpIp_Connector :
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Connector,ACE_SOCK_STREAM,2048>
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Connector,ACE_SOCK_STREAM,2048> connector_base;

public:
	OONet_TcpIp_Connector();

	void set_protocol(OONet_TcpIp_Manager* p);

	int close(u_long flags = 0);

protected:
	ssize_t send_n(ACE_Message_Block* mb)
	{
		return peer().send_n(mb);
	}

private:
	OONet_TcpIp_Manager* m_protocol;

	int on_close();
};

#endif // _OONET_TCPIP_CONNECTOR_H_INCLUDED_
