#include "./TcpIp_Connector.h"

#include "./TcpIp_Manager.h"

TcpIp_Connector::TcpIp_Connector() :
	m_protocol(0)
{
}

void 
TcpIp_Connector::set_protocol(TcpIp_Manager* p)
{
	m_protocol = p;
}

ssize_t 
TcpIp_Connector::send_n(ACE_Message_Block* mb)
{
	return peer().send_n(mb);
}

int 
TcpIp_Connector::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
	if (m_protocol)
		m_protocol->remove_connection(this);
	
	return svc_base::handle_close(fd,mask);
}
