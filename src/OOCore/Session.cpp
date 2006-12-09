#include "OOCore_precomp.h"

#include "./Session.h"

void Session::Connect()
{
	pid_t pid = ACE_INVALID_PID;

	// Open the Server key file
	ACE_HANDLE file = ACE_OS::open(GetBootstrapFileName().c_str(),O_RDONLY);
	if (file != INVALID_HANDLE_VALUE)
	{
		if (ACE_OS::read(file,&pid,sizeof(pid)) == sizeof(pid))
		{
            // Check if the process is still running...
			if (ACE::process_active(pid)!=1)
			{
				pid = ACE_INVALID_PID;
			}
		}
	}
		
	if (pid == ACE_INVALID_PID)
	{
		if (file != ACE_INVALID_HANDLE)
			ACE_OS::close(file);

		// Launch the server
		
		// Find what the server is called
		ACE_TString strExec;
		if (strExec.empty())
			strExec = ACE_OS::getenv("OOSERVER");
		if (strExec.empty())
			strExec = "OOServer";

		// Set the process options
		ACE_Process_Options options;
		options.avoid_zombies(0);
		options.handle_inheritence(0);
		if (options.command_line(strExec.c_str()) == -1)
			OOCORE_THROW_ERRNO(ACE_OS::last_error() ? ACE_OS::last_error() : EINVAL);

		// Set the creation flags
		u_long flags = 0;
#if defined (ACE_WIN32) && defined(_DEBUG)
		flags |= CREATE_NEW_CONSOLE;
#endif
		options.creation_flags(flags);

		// Spawn the process
		ACE_Process process;
		if (process.spawn(options)==ACE_INVALID_PID)
			OOCORE_THROW_LASTERROR();

		// Wait 1 second for the process to launch, if it takes more than 10 seconds its probably okay
		ACE_exitcode exitcode = 0;
		int ret = process.wait(ACE_Time_Value(10),&exitcode);
		if (ret==-1)
			OOCORE_THROW_LASTERROR();

		if (ret!=0)
			OOCORE_THROW_ERRNO(ret);

		// Re-open file
		file = ACE_OS::open(GetBootstrapFileName().c_str(),O_RDONLY);
		if (file == INVALID_HANDLE_VALUE)
		{
			int err = ACE_OS::last_error();
			process.kill();
			OOCORE_THROW_ERRNO(err);
		}
	
		// Read pid again
		if (ACE_OS::read(file,&pid,sizeof(pid)) != sizeof(pid))
		{
			int err = ACE_OS::last_error();
			process.kill();
			ACE_OS::close(file);
			OOCORE_THROW_ERRNO(err);
		}

		// Check the pids match
		if (pid != process.getpid())
		{
			process.kill();
			ACE_OS::close(file);
			OOCORE_THROW_ERRNO(EINVAL);
		}
	}

	// Get the port number from the binding
	u_short uPort;
	if (ACE_OS::read(file,&uPort,sizeof(uPort)) != sizeof(uPort))
	{
		int err = ACE_OS::last_error();
		ACE_OS::close(file);
		OOCORE_THROW_ERRNO(err);
	}
	ACE_OS::close(file);
	
	// Sort out addresses
	ACE_INET_Addr addr(uPort,ACE_LOCALHOST);

	// Connect to the OOServer main process...
	ACE_SOCK_Stream peer;
	if (ACE_SOCK_Connector().connect(peer,addr) == -1)
		OOCORE_THROW_LASTERROR();

	// Send our uid or pid
	Session::Request request = {0};
	request.cbSize = sizeof(request);
#ifdef OMEGA_WIN32
	request.uid = ACE_OS::getpid();
#else
	request.uid = ACE_OS::getuid();
#endif
	peer.send(&request,request.cbSize);

	// Wait for the response to come back...
	ACE_Time_Value wait(5);
	Session::Response response = {0};
	if (peer.recv(&response.cbSize,sizeof(response.cbSize),&wait) < sizeof(response.cbSize))
		OOCORE_THROW_LASTERROR();

	// Check the response is valid
	if (response.cbSize < sizeof(response))
		OOCORE_THROW_ERRNO(EINVAL);

	// Recv the rest...
	if (peer.recv(&response.bFailure,response.cbSize - sizeof(response.cbSize)) < static_cast<ssize_t>(response.cbSize - sizeof(response.cbSize)))
		OOCORE_THROW_LASTERROR();

	// Check failure code
	if (response.bFailure)
		OOCORE_THROW_ERRNO(response.err);
	
/*	// Connect to an instance of LocalTransport
	OTL::Remoting::ACE::Transport_Svc_Connector<OTL::SingletonObjectImpl<LocalTransport>, ACE_SOCK_CONNECTOR> connector(Engine::GetSingleton()->get_reactor());

#if defined (ACE_WIN32) || !defined (_ACE_USE_SV_SEM)
	connector.connector().preferred_strategy(ACE_MEM_IO::MT);
#endif
	
	OTL::SingletonObjectImpl<LocalTransport>* pTransport = 0;
	if (connector.connect(pTransport,addr)!=0)
	{
		Engine::GetSingleton()->release_reactor();
		OOCORE_THROW_LASTERROR();
	}	

	// Stash the transport pointer in a ObjectPtr
	OTL::ObjectPtr<OTL::SingletonObjectImpl<LocalTransport> > ptrTransport;
	ptrTransport.Attach(pTransport);

	// LocalTransport maintains its own ref count on the reactor
	Engine::GetSingleton()->release_reactor();

	// Attach a StdObjectManager to the LocalTransport
	OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM(OID_StdObjectManager);
	ptrOM->Attach(ptrTransport);
	
	// Prepare the static interface for the server end of OOServer::IInterProcess
	OTL::ObjectPtr<IObject> ptrObj;
	ptrObj.Attach(ptrOM->PrepareStaticInterface(OOServer::OID_InterProcess,OOServer::IID_IInterProcess));
	OTL::ObjectPtr<OOServer::IInterProcess> ptrIPC(ptrObj);
	*/
	
	// Now we need to get various things like the ROT etc...
	void* TODO;
}
