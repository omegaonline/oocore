#ifndef _OONET_TCPIP_CONNECTOR_H_INCLUDED_
#define _OONET_TCPIP_CONNECTOR_H_INCLUDED_

#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"
#include "../OOSvc/Transport_Connector.h"

class OONet_TcpIp_Connector :
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Connector,ACE_SOCK_STREAM,2048>
{
};

#endif // _OONET_TCPIP_CONNECTOR_H_INCLUDED_
