#include "./TcpIp_Acceptor.h"

#include "./TcpIp_Manager.h"

TcpIp_Acceptor::TcpIp_Acceptor() :
	m_protocol(0)
{
}

int 
TcpIp_Acceptor::open(void* p)
{
	m_protocol = reinterpret_cast<TcpIp_Manager*>(p);
	if (m_protocol == 0)
		return -1;

	if (svc_base::open(p) != 0)
		return -1;

	ACE_INET_Addr addr;
	if (peer().get_local_addr(addr) == 0)
		m_protocol->add_connection(addr,this);
	
	return 0;
}

ssize_t 
TcpIp_Acceptor::send_n(ACE_Message_Block* mb)
{
	return peer().send_n(mb);
}

int 
TcpIp_Acceptor::handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask)
{
	if (m_protocol)
		m_protocol->remove_connection(this);
	
	return svc_base::handle_close(fd,mask);
}
