/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#include "./RootManager.h"
#include "./SpawnedProcess.h"

#include <OOCore/Preprocessor/base.h>
#include <ace/OS.h>

RootManager::RootManager() : 
	ACE_Acceptor<RootConnection, ACE_SOCK_ACCEPTOR>(),
	m_config_file(ACE_INVALID_HANDLE)
{
}

// This function must match the one in OOCore/Root.cpp
/*static ACE_TString GetBootstrapFileName()
{
#define BOOTSTRAP_FILE "ooserver.bootstrap"

#ifdef ACE_WIN32

	ACE_TCHAR szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPath(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if FAILED(hr)
		return ACE_TString(ACE_TEXT(OMEGA_CONCAT("C:\\",BOOTSTRAP_FILE)));

	ACE_TCHAR szBuf2[MAX_PATH] = {0};
	if (!PathCombine(szBuf2,szBuf,ACE_TEXT(BOOTSTRAP_FILE)))
		return ACE_TString(ACE_TEXT(OMEGA_CONCAT("C:\\",BOOTSTRAP_FILE)));

	return ACE_TString(szBuf2);

#else

#define BOOTSTRAP_PREFIX ACE_TEXT("/tmp/")
	
	return ACE_TString(ACE_TEXT(OMEGA_CONCAT(BOOTSTRAP_PREFIX,BOOTSTRAP_FILE)));

#endif
}*/

int RootManager::open()
{
    // Open the Server key file
	if (m_config_file != ACE_INVALID_HANDLE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Already open.\n")),-1);

	m_config_file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_RDONLY);
	if (m_config_file != INVALID_HANDLE_VALUE)
	{
		pid_t pid;
		if (ACE_OS::read(m_config_file,&pid,sizeof(pid)) == sizeof(pid))
		{
			// Check if the process is still running...
			if (ACE::process_active(pid)==1)
			{
				// Already running on this machine... Fail
				ACE_OS::close(m_config_file);
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOServer already running.\n")),-1);
			}
		}
		ACE_OS::close(m_config_file);
	}

	m_config_file = ACE_OS::open(Session::GetBootstrapFileName().c_str(),O_WRONLY | O_CREAT | O_TRUNC | O_TEMPORARY);
	if (m_config_file == INVALID_HANDLE_VALUE)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")),-1);

	// Write our pid instead
	pid_t pid = ACE_OS::getpid();
	if (ACE_OS::write(m_config_file,&pid,sizeof(pid)) != sizeof(pid))
	{
		ACE_OS::close(m_config_file);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")),-1);
	}

#if defined(ACE_WIN32)

	::DebugBreak();

#endif

	// Bind a tcp socket via acceptor
	ACE_INET_Addr port_addr((u_short)0);
	if (ACE_Acceptor<RootConnection, ACE_SOCK_ACCEPTOR>::open(port_addr,ACE_Reactor::instance()) == -1)
	{
		ACE_OS::close(m_config_file);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")),-1);
	}
	
	// Get our port number
	if (acceptor().get_local_addr(port_addr)==-1)
	{
		ACE_OS::close(m_config_file);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")),-1);
	}
	
	// Write it back...
	u_short uPort = port_addr.get_port_number();
	if (ACE_OS::write(m_config_file,&uPort,sizeof(uPort)) != sizeof(uPort))
	{
		ACE_OS::close(m_config_file);
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")),-1);
	}

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening for client connections on port %u.\n"),uPort));
	
	return 0;
}

int RootManager::close()
{
	// Stop accepting new connections
	int ret = ACE_Acceptor<RootConnection, ACE_SOCK_ACCEPTOR>::close();
	if (ret != 0)
		return -1;

	if (m_config_file != ACE_INVALID_HANDLE)
		ACE_OS::close(m_config_file);

	return 0;
}

void RootManager::connect_client(const Session::Request& request, Session::Response& response)
{
	// Set the response size
	response.cbSize = sizeof(response);

	// Alloc a new SpawnedProcess
	SpawnedProcess* pProcess = 0;
	ACE_NEW_NORETURN(pProcess,SpawnedProcess);
	if (!pProcess)
	{
		response.bFailure = 1;
		response.err = E_OUTOFMEMORY;
		return;
	}

	// Open an acceptor
	ACE_INET_Addr addr((u_short)0);
	ACE_SOCK_Acceptor acceptor;
	int ret = acceptor.open(addr);
	if (ret != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")));
	}
	else
	{
		// Get the port we are accepting on
		if (acceptor.get_local_addr(addr)!=0)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("get_local_addr() failed")));
		}
		else
		{
			// Spawn the user process
			ret = pProcess->Spawn(request.uid,addr.get_port_number());
			if (ret != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("spawn() failed")));
			}
			else
			{
				// Accept a socket
				ACE_SOCK_Stream stream;
				ACE_Time_Value wait(2);
				ret = acceptor.accept(stream,0,&wait);
				if (ret != 0)
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("accept() failed")));
				}
				else
				{
					// Read the port the user session is listening on...
					wait.sec(2);
					ret = stream.recv(&response.uNewPort,sizeof(response.uNewPort),&wait);
					if (ret != 0)
					{
						ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("accept() failed")));
					}
					else
					{
						response.bFailure = 0;
					}

					void* TODO;	// Attach the socket to a Svc_handler....
					stream.close();
				}

				if (ret != 0)
					pProcess->Close();
			}
		}

		acceptor.close();
	}

	if (ret != 0)
	{
		delete pProcess;
		response.bFailure = 1;
		response.err = ret;
	}	
}
