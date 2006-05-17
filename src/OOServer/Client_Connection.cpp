#include "./Client_Connection.h"

namespace OOCore
{
namespace Impl
{
	OOCore_Export int RegisterAsServer();
};
};

int 
Client_Connection::init(void)
{
	// Init the OOCore
	if (OOCore::Impl::RegisterAsServer() != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("RegisterAsServer failed.\n")),-1);
	
	// Open the Server key
	OOUtil::RegistryKey_Ptr ptrKey;
	if (ptrKey.open("Server//CurrentSession",true) != 0)
	{
		errno = EHOSTUNREACH;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No local server registered\n")),-1);
	}

	// Get the stored pid
	ACE_CString strPid;
	if (ptrKey.QueryValue("Pid",strPid) == 0)
	{
		// Check if the process is still running...
		pid_t pid = ACE_OS::atoi(strPid.c_str());
		if (ACE::process_active(pid)==1)
		{
			// Already running on this machine... Fail
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
		}

		// Remove the pid
		ptrKey->RemoveValue("Pid");
	}

	// Bind our pid instead
	if (ptrKey.SetValue("Pid",ACE_OS::getpid()) != 0)
		return -1;

	// Rebind our file location
			
#if (defined (ACE_WIN32))
	ACE_TCHAR this_exe[MAXPATHLEN];
	if (ACE_TEXT_GetModuleFileName(0, this_exe, MAXPATHLEN) == 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to determine own process name\n")),-1);
		
	ptrKey->SetValue("ExeName",this_exe);
#else
	//const char* this_exe = exec();
#endif

/*#if defined (ACE_WIN32) || !defined (_ACE_USE_SV_SEM)
	ACCEPTOR::instance()->acceptor().preferred_strategy(ACE_MEM_IO::MT);
#endif*/
    
	ACE_MEM_Addr port_addr((u_short)0);
	if (ACCEPTOR::instance()->open(port_addr,OOCore::GetEngineReactor()) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	if (ACCEPTOR::instance()->acceptor().get_local_addr(port_addr)==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);
	
	if (ptrKey.SetValue("LocalPort",port_addr.get_port_number()) != 0)
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
