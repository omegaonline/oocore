#include "./LocalTransport.h"

#include <ace/MEM_Connector.h>
#include <ace/Connector.h>

int 
OOCore::Impl::LocalTransport::init(void)
{
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
			// A process is already running
			return 0;
		}

		// Remove the pid
		ptrKey->RemoveValue("Pid");
	}

	// Launch the server
	ACE_CString strExeName;
	ptrKey.QueryValue("ExeName",strExeName);
	if (launch_server(strExeName)==-1)
		return -1;

	// Get the port number from the binding
	ACE_CString strPort;
	if (ptrKey.QueryValue("LocalPort",strPort) != 0)
	{
		errno = EHOSTUNREACH;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No local port registered\n")),-1);
	}
	u_short uPort = ACE_OS::atoi(strPort.c_str());
	
	// Sort out addresses
	ACE_INET_Addr addr(uPort,ACE_MEM_Addr().get_ip_address());

	// Keep us alive artifically because we are a singleton
	LOCAL_TRANSPORT::instance()->AddRef();

	// Connect to an instance of LocalTransport
	ACE_Connector<LocalTransport, ACE_MEM_CONNECTOR> connector(OOCore::GetEngineReactor());

/*#if defined (ACE_WIN32) || !defined (_ACE_USE_SV_SEM)
	connector.connector().preferred_strategy(ACE_MEM_IO::MT);
#endif*/
	
	LocalTransport* pThis = LOCAL_TRANSPORT::instance();
	if (connector.connect(pThis,addr)!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to connect to server: %m [%s]\n"),strExeName.c_str()),-1);

	// We need to be properly connected locally...
	if (pThis->wait_for_connect())
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connecting timed out and gave up\n")),-1);

	return 0;
}

int 
OOCore::Impl::LocalTransport::launch_server(ACE_CString& strExeName)
{
	// Find what the server is called
	if (strExeName.length() == 0)
		strExeName = ACE_OS::getenv("OOSERVER");
		
	if (strExeName.length() == 0)
		strExeName = ACE_TEXT("OOServer");

	return ExecProcess(strExeName);	
}

int 
OOCore::Impl::LocalTransport::recv(ACE_Message_Block*& mb, ACE_Time_Value* wait)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	return svc_base::recv(mb,wait);
}

ssize_t 
OOCore::Impl::LocalTransport::send_n(ACE_Message_Block* mb)
{
	return this->peer().send(mb,0);
}
