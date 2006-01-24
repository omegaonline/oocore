#ifndef _OONET_TCPIP_ACCEPTOR_H_INCLUDED_
#define _OONET_TCPIP_ACCEPTOR_H_INCLUDED_

#include <ace/Acceptor.h>
#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"

class TcpIp_Manager;

class TcpIp_Acceptor : 
	public OOCore::Transport_Svc_Handler<ACE_SOCK_STREAM,2048>
{
	typedef OOCore::Transport_Svc_Handler<ACE_SOCK_STREAM,2048> svc_base;
	
public:
	TcpIp_Acceptor();
	
	int open(void* p);

protected:
	virtual ~TcpIp_Acceptor() {}

	void Closed();

private:
	ssize_t send_n(ACE_Message_Block* mb);
	
	OOCore::Object_Ptr<TcpIp_Manager> m_protocol;
};

#endif // _OONET_TCPIP_ACCEPTOR_H_INCLUDED_
