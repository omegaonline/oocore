#include "./Client_Connection.h"

#include <ace/Acceptor.h>
#include <ace/MEM_Acceptor.h>
#include <ace/Singleton.h>
#include <ace/Mutex.h>

#include "../OOCore/Engine.h"

namespace OOCore
{
namespace Impl
{
	OOCore_Export int RegisterAsServer();
	OOCore_Export int SetServerPort(OOObject::uint16_t uPort);
};
};

int 
Client_Connection::init(void)
{
	// Init the OOCore
	int ret = OOCore::Impl::RegisterAsServer();
	if (ret == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("RegisterAsServer failed.\n")),-1);
	else if (ret==1)
	{
		// Already running on this machine... Fail
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
	}

	ACE_MEM_Addr port_addr((u_short)0);
	if (ACE_Singleton<ACE_Acceptor<Client_Connection, ACE_MEM_ACCEPTOR>, ACE_Thread_Mutex>::instance()->open(port_addr,OOCore::ENGINE::instance()->reactor()) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	if (ACE_Singleton<ACE_Acceptor<Client_Connection, ACE_MEM_ACCEPTOR>, ACE_Thread_Mutex>::instance()->acceptor().get_local_addr(port_addr)==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);
	
	OOObject::uint16_t uPort = port_addr.get_port_number();
	if (OOCore::Impl::SetServerPort(uPort) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to set local port")),-1);

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening on port %u for client connections.\n"),uPort));
		
	return 0;
}

ssize_t 
Client_Connection::send_n(ACE_Message_Block* mb)
{
	return this->peer().send(mb,0);
}
