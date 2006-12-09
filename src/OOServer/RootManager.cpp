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

#include <ace/OS.h>
#include <ace/SOCK_Acceptor.h>

#include <OOCore/Preprocessor/base.h>

RootManager::RootManager() : 
	LocalAcceptor<ClientConnection>(),
	m_config_file(ACE_INVALID_HANDLE)
{
}

int RootManager::init()
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

	// Bind a tcp socket 
	ACE_INET_Addr sa((u_short)0,(ACE_UINT32)INADDR_LOOPBACK);
	if (open(sa) != 0)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("open() failed")));
		ACE_OS::close(m_config_file);
		return -1;
	}
	
	// Get our port number
	int len = sa.get_size ();
	sockaddr* addr = reinterpret_cast<sockaddr*>(sa.get_addr());
	if (ACE_OS::getsockname(this->get_handle(),addr,&len) == -1)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Failed to discover local port")));
		ACE_OS::close(m_config_file);
		close();		
		return -1;
	}
	sa.set_type(addr->sa_family);
	sa.set_size(len);

	// Write it back...
	u_short uPort = sa.get_port_number();
	if (ACE_OS::write(m_config_file,&uPort,sizeof(uPort)) != sizeof(uPort))
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("write() failed")));
		ACE_OS::close(m_config_file);
		close();
		return -1;
		
	}

	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening for client connections on port %u.\n"),uPort));
	
	return 0;
}

void RootManager::close()
{
	ACE_GUARD(ACE_Thread_Mutex,guard,m_lock);

	// Don't accept any more...
	reissue_accept(0);

	// Kill the map
	for (std::map<SpawnedProcess::USERID,UserProcess>::iterator i=m_mapSpawned.begin();i!=m_mapSpawned.end();++i)
	{
		delete i->second.pSpawn;
	}
	m_mapSpawned.clear();

	// Close the config file...
	if (m_config_file != ACE_INVALID_HANDLE)
	{
		ACE_OS::close(m_config_file);
		m_config_file = ACE_INVALID_HANDLE;
	}
}

void RootManager::spawn_client(const Session::Request& request, Session::Response& response, UserProcess& process)
{
	// Alloc a new SpawnedProcess
	ACE_NEW_NORETURN(process.pSpawn,SpawnedProcess);
	if (!process.pSpawn)
	{
		response.bFailure = 1;
		response.err = E_OUTOFMEMORY;
		return;
	}

	// Open an acceptor
	ACE_INET_Addr addr((u_short)0);
	ACE_SOCK_Acceptor acceptor;
	int ret = acceptor.open(addr,0,PF_INET,1);
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
			ret = process.pSpawn->Spawn(request.uid,addr.get_port_number());
			if (ret != 0)
			{
				ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("spawn() failed")));
			}
			else
			{
				// Accept a socket
				ACE_SOCK_Stream stream;
				ACE_Time_Value wait(5);
				ret = acceptor.accept(stream,0,&wait);
				if (ret != 0)
				{
					ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("accept() failed")));
				}
				else
				{
					// Read the port the user session is listening on...
					if (stream.recv(&response.uNewPort,sizeof(response.uNewPort),&wait) < sizeof(response.uNewPort))
					{
						ACE_ERROR((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("recv() failed")));
						ret = -1;
					}
					else
					{
						response.bFailure = 0;
					}

					void* TODO;	// Attach the socket to a Svc_handler....
					stream.close();
				}

				if (ret != 0)
					process.pSpawn->Close();
			}
		}

		acceptor.close();
	}

	if (ret != 0)
	{
		delete process.pSpawn;
		response.bFailure = 1;
		response.err = ret;
	}
	else
	{
		response.bFailure = 0;
		process.uPort = response.uNewPort;
	}
}

void RootManager::connect_client(const Session::Request& request, Session::Response& response)
{
	// Set the response size
	response.cbSize = sizeof(response);

	SpawnedProcess::USERID key;
	int err = SpawnedProcess::ResolveTokenToUid(request.uid,key);
	if (err != 0)
	{
		response.bFailure = 1;
		response.err = err;
		return;
	}

	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,
		response.bFailure = 1;
		response.err = ACE_OS::last_error();
		return;
	);

	// See if we have a process already
	std::map<SpawnedProcess::USERID,UserProcess>::iterator i=m_mapSpawned.find(key);
	if (i!=m_mapSpawned.end())
	{
		// See if the process is still alive
		if (!i->second.pSpawn->IsRunning())
		{
			// Remove it
			delete i->second.pSpawn;
			m_mapSpawned.erase(i);
			i = m_mapSpawned.end();
		}
	}

	if (i==m_mapSpawned.end())
	{
		// No we don't
		UserProcess process;
		spawn_client(request,response,process);

		if (!response.bFailure)
			m_mapSpawned.insert(std::map<SpawnedProcess::USERID,UserProcess>::value_type(key,process));
	}
	else
	{
		// Yes we do
		response.bFailure = 0;
		response.uNewPort = i->second.uPort;
	}
}
