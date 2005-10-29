#include "./TcpIp_Connector.h"

#include "./TcpIp_Manager.h"

OONet_TcpIp_Connector::OONet_TcpIp_Connector() :
	m_protocol(0)
{
}

void OONet_TcpIp_Connector::set_protocol(OONet_TcpIp_Manager* p)
{
	m_protocol = p;
}

int OONet_TcpIp_Connector::on_close()
{
	if (m_protocol)
		m_protocol->transport_closed(this);

	return connector_base::on_close();
}