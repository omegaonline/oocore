#include "./TcpIp_Acceptor.h"

#include "./TcpIp_Manager.h"

OONet_TcpIp_Acceptor::OONet_TcpIp_Acceptor() :
	m_protocol(0)
{
}

int OONet_TcpIp_Acceptor::open(void* p)
{
	m_protocol = reinterpret_cast<OONet_TcpIp_Manager*>(p);
	if (m_protocol == 0)
		return -1;

	if (acceptor_base::open(p) != 0)
		return -1;

	ACE_INET_Addr addr;
	if (peer().get_remote_addr(addr) == 0)
	{
		ACE_TCHAR buffer[MAXHOSTNAMELEN];
		addr.addr_to_string(buffer,MAXHOSTNAMELEN);

		m_protocol->transport_accepted(this,buffer);
	}

	return 0;
}

int OONet_TcpIp_Acceptor::request_close()
{
	if (m_protocol)
		m_protocol->transport_closed(this);

	return acceptor_base::request_close();
}

void OONet_TcpIp_Acceptor::handle_shutdown()
{
	request_close();
}
