#include "./Binding.h"

#include <ace/OS.h>
#include <ace/Process.h>

OOCore_Binding::OOCore_Binding() :
	m_unbind_pid(false)
{
#if (defined (ACE_WIN32) && defined (UNICODE))
	m_context.name_options()->use_registry(1);
	m_context.name_options()->namespace_dir(ACE_TEXT("SOFTWARE\\OmegaOnline"));
#endif

	m_context.name_options()->context(ACE_Naming_Context::NODE_LOCAL);
	m_context.open(m_context.name_options()->context(),1);
}

OOCore_Binding::~OOCore_Binding()
{
	if (m_unbind_pid)
		m_context.unbind(ACE_TEXT("pid"));

	m_context.close_down();
}

int OOCore_Binding::launch(bool bAsServer)
{
	// Get the stored pid
	ACE_TCHAR* pszType = NULL;
	ACE_NS_WString strPid;
	int ret = m_context.resolve(ACE_TEXT("pid"),strPid,pszType);
	ACE_OS::free(pszType);
	if (ret==0)
	{
		// Check if the process is still running...
		pid_t pid = ACE_OS::atoi(strPid.c_str());
		if (ACE::process_active(pid)==1)
		{
			// A process is already running
			return (bAsServer ? 1 : 0);
		}

		// Remove the pid
		if (m_context.unbind(ACE_TEXT("pid"))==-1)
			return -1;
	}
	
	if (bAsServer)
	{
		// Bind our pid instead
		ACE_TCHAR szBuf[24];
		ACE_OS::sprintf(szBuf,ACE_TEXT("%d"),ACE_OS::getpid());
		if (m_context.bind(ACE_TEXT("pid"),szBuf)!=0)
			return -1;

#if (defined (ACE_WIN32))
		// Rebind our file location
		ACE_TCHAR this_exe[MAXPATHLEN];
		if (ACE_TEXT_GetModuleFileName(0, this_exe, MAXPATHLEN) != 0)
			m_context.rebind(ACE_TEXT("server"),this_exe);
#endif

		m_unbind_pid = true;
	}
	else
	{
		if (launch_server()==-1)
			return -1;
	}

	return 0;
}

int OOCore_Binding::launch_server()
{
	// Find what the server is called
	ACE_TString exe_name;
#if defined (ACE_WIN32)
	ACE_TCHAR* pszType = NULL;
	ACE_NS_WString strExeName;
	if (m_context.resolve(ACE_TEXT("server"),strExeName,pszType)==0)
	{
		ACE_OS::free(pszType);
		exe_name = ACE_TEXT_WCHAR_TO_TCHAR(strExeName.c_str());
	}
#else
	exe_name = ACE_OS::getenv("OOSERVER");
#endif

	if (exe_name.length() == 0)
		exe_name = ACE_TEXT("OOServer");

	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(1);
	options.handle_inheritence(0);
	if (options.command_line(exe_name.c_str()) == -1)
		return -1;

	// Set the creation flags
	u_long flags = ACE_Process_Options::NO_EXEC;
#if defined (ACE_WIN32)
	flags |= CREATE_NEW_CONSOLE;//DETACHED_PROCESS;
#endif
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		return -1;

	// Wait 1 second for the process to launch, if it takes more than 1 second its probably okay
	ACE_exitcode exitcode;
	int ret = process.wait(ACE_Time_Value(1),&exitcode);
	if (ret==-1)
		return -1;

	if (ret!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Process exited with code %d.\n"),exitcode),-1);

    return 0;
}

const ACE_TCHAR* OOCore_Binding::dll_name(void)
{
	return ACE_TEXT("OOCore");
}

const ACE_TCHAR* OOCore_Binding::name(void)
{
	return ACE_TEXT("OOCore_Binding");
}

int OOCore_Binding::find(const ACE_TCHAR* name, ACE_NS_WString& value)
{
	ACE_TCHAR* pszType = NULL;
	int ret = m_context.resolve(name,value,pszType);
	ACE_OS::free(pszType);
	return ret;
}

int OOCore_Binding::rebind(const ACE_TCHAR* name, const ACE_NS_WString& value)
{
	// Use our port
	return m_context.rebind(name,value);
}
