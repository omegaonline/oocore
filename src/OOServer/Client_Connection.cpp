#include "./Client_Connection.h"

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

/*#if defined (ACE_WIN32) || !defined (_ACE_USE_SV_SEM)
	ACCEPTOR::instance()->acceptor().preferred_strategy(ACE_MEM_IO::MT);
#endif*/
    
	ACE_MEM_Addr port_addr((u_short)0);
	if (ACCEPTOR::instance()->open(port_addr,OOCore::GetEngineReactor()) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	if (ACCEPTOR::instance()->acceptor().get_local_addr(port_addr)==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);
	
	OOObject::uint16_t uPort = port_addr.get_port_number();
	if (OOCore::Impl::SetServerPort(uPort) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to set local port")),-1);

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening for client connections.\n")));
		
	return 0;
}

int 
Client_Connection::recv(ACE_Message_Block*& mb, ACE_Time_Value* wait)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	return svc_base::recv(mb,wait);
}

ssize_t 
Client_Connection::send_n(ACE_Message_Block* mb)
{
	return this->peer().send(mb,0);
}
