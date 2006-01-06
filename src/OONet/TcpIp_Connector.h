#ifndef _OONET_TCPIP_CONNECTOR_H_INCLUDED_
#define _OONET_TCPIP_CONNECTOR_H_INCLUDED_

#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"

class TcpIp_Manager;

class TcpIp_Connector :
	public OOCore::Transport_Svc_Handler<false,ACE_SOCK_STREAM,2048>
{
	typedef OOCore::Transport_Svc_Handler<false,ACE_SOCK_STREAM,2048> svc_base;

public:
	TcpIp_Connector();

	void set_protocol(TcpIp_Manager* p);

	int handle_close(ACE_HANDLE fd = ACE_INVALID_HANDLE, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK);
	
private:
	OOCore::Object_Ptr<TcpIp_Manager> m_protocol;

	ssize_t send_n(ACE_Message_Block* mb);
};

#endif // _OONET_TCPIP_CONNECTOR_H_INCLUDED_
