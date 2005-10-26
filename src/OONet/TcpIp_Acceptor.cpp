#include "./TcpIp_Acceptor.h"

#include "./TcpIp_Manager.h"

OONet_TcpIp_Acceptor::OONet_TcpIp_Acceptor() :
	m_protocol(0)
{
}

int OONet_TcpIp_Acceptor::open(void* p)
{
	m_protocol = reinterpret_cast<OONet_TcpIp_Manager*>(p);
	
	return acceptor_base::open(p);
}

int OONet_TcpIp_Acceptor::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
	if (m_protocol)
		m_protocol->transport_closed(this);

	return acceptor_base::handle_close(fd,mask);
}

void OONet_TcpIp_Acceptor::handle_shutdown()
{
	close();
}
