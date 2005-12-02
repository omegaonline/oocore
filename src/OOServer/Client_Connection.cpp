#include "./Client_Connection.h"

#include "../OOCore/Engine.h"

int 
Client_Connection::init(void)
{
	// Init the OOCore
	int ret = OOCore::InitAsServer();
	if (ret == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("InitAsServer failed.\n")),-1);
	else if (ret==1)
	{
		// Already running on this machine... Fail
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
	}

	acceptor_type* acceptor;
	ACE_NEW_RETURN(acceptor,acceptor_type(OOCore::ENGINE::instance()->reactor()),-1);

	ACE_MEM_Addr port_addr((u_short)0);
	if (acceptor->open(port_addr,OOCore::ENGINE::instance()->reactor()) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	if (acceptor->acceptor().get_local_addr(port_addr)==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);
	
	OOObject::uint16_t uPort = port_addr.get_port_number();
	if (OOCore::SetServerPort(uPort) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to set local port")),-1);

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening on port %u for client connections.\n"),uPort));
		
	return 0;
}

ssize_t 
Client_Connection::send_n(ACE_Message_Block* mb)
{
	return this->peer().send(mb,0);
}
