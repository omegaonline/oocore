#include "./Connection_Manager.h"

#include <ace/MEM_Connector.h>
#include <ace/Connector.h>

#include "./Binding.h"
#include "./Engine.h"

int 
OOCore::Impl::Connection_Manager::init(void)
{
	if (BINDING::instance()->launch(false) != 0)
		return -1;

	// Get the port number from the binding
	ACE_NS_WString strPort;
	if (BINDING::instance()->find(ACE_TEXT("local_port"),strPort) != 0)
	{
		errno = EHOSTUNREACH;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No local port registered\n")),-1);
	}
	
	u_short uPort = ACE_OS::atoi(strPort.c_str());
	
	// Sort out addresses
	ACE_INET_Addr addr(uPort,ACE_MEM_Addr().get_ip_address());

	// Keep us alive artifically because we are a singleton
	CONNECTION_MANAGER::instance()->AddRef();

	// Connect to an instance of OOCore_Connection_Manager
	ACE_Connector<Connection_Manager, ACE_MEM_CONNECTOR> connector(OOCore::ENGINE::instance()->reactor());
	Connection_Manager* pThis = CONNECTION_MANAGER::instance();
	if (connector.connect(pThis,addr)!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to connect to server: %m\n")),-1);

	return 0;
}

ssize_t 
OOCore::Impl::Connection_Manager::send_n(ACE_Message_Block* mb)
{
	return this->peer().send(mb,0);
}
