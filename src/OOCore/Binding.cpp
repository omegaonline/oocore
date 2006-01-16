#include "./Binding.h"

#include <ace/OS.h>
#include <ace/Process.h>

#ifdef ACE_WIN32
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include "./OOCore_Impl.h"

OOCore::Impl::Binding::Binding() :
	m_unbind_pid(false), m_bOpen(false)
{
	m_context.name_options()->database(ACE_TEXT("OmegaOnline.reg_db"));
	
#ifdef ACE_WIN32
#ifdef UNICODE
	m_context.name_options()->use_registry(1);
	m_context.name_options()->namespace_dir(ACE_TEXT("SOFTWARE\\OmegaOnline"));
#else
	char szBuf[MAX_PATH] = {0};
	if (::SHGetSpecialFolderPathA(NULL,szBuf,CSIDL_COMMON_APPDATA,0))
	{
		::PathAppendA(szBuf,"OmegaOnline");
		if (!::PathFileExistsA(szBuf))
		{
			if (ACE_OS::mkdir(szBuf) != 0)
				goto errored;
		}
		else if (!::PathIsDirectoryA(szBuf))
			goto trylocal;
	}
	else
	{
trylocal:
		if (::GetModuleFileNameA(OOCore::Impl::g_hInstance,szBuf,MAX_PATH)!=0)
		{
			::PathRemoveFileSpecA(szBuf);
		}
	}
	
	if (szBuf[0] == 0)
	{
errored:
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Failed to detect registry database location\n")));
		ACE_OS::abort();
	}

	m_context.name_options()->namespace_dir(szBuf);

#if (defined (ACE_HAS_WINNT4) && ACE_HAS_WINNT4 != 0)

	// Use a different base address
	m_context.name_options()->base_address((char*)(1024UL*1024*512));

	// Sometimes the base address is already in use - CLR for example
	// So we check it first - I wish ACE would do this for us!
	// The problem with just defaulting to address 0x0 - which mean pick any,
	// is that ACE seems to crash creating the MEM_Map for the first time!
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(m_context.name_options()->base_address(),&mbi,sizeof(mbi)))
	{
		if (mbi.State != MEM_FREE)
		{
			// Please record which addresses aren't useful!, e.g.
			//
			// 0x04000000	-	Used by VB.NET
			//
			ACE_OS::abort();		
		}
	}	
#endif

#endif
#else
	// HUGE HACK TO GET THIS WORKING UNDER *NIX
	m_context.name_options()->namespace_dir(ACE_TEXT("/tmp/"));
#endif

	m_context.name_options()->context(ACE_Naming_Context::NODE_LOCAL);
}

OOCore::Impl::Binding::~Binding()
{
	if (m_unbind_pid)
		m_context.unbind(ACE_TEXT("pid"));

	m_context.close_down();
}

int 
OOCore::Impl::Binding::check_open()
{
	if (m_bOpen)
		return 0;

	if (m_context.open(m_context.name_options()->context(),1) != 0)
		return -1;
		
	m_bOpen = true;
	return 0;
}

int 
OOCore::Impl::Binding::launch(bool bAsServer)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	// Get the stored pid
	ACE_TCHAR* pszType = 0;
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

		// Rebind our file location
				
#if (defined (ACE_WIN32))
		ACE_TCHAR this_exe[MAXPATHLEN];
		if (ACE_TEXT_GetModuleFileName(0, this_exe, MAXPATHLEN) == 0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to determine own process name\n")),-1);
			
		m_context.rebind(ACE_TEXT("server"),this_exe);
#else
		//const char* this_exe = exec();
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

int 
OOCore::Impl::Binding::launch_server()
{
	// Find what the server is called
	ACE_TString exe_name;
	ACE_TCHAR* pszType = 0;
	ACE_NS_WString strExeName;
	if (m_context.resolve(ACE_TEXT("server"),strExeName,pszType)==0)
	{
		ACE_OS::free(pszType);
		exe_name = ACE_TEXT_WCHAR_TO_TCHAR(strExeName.c_str());
	}

	if (exe_name.length() == 0)
		exe_name = ACE_OS::getenv("OOSERVER");
		
	if (exe_name.length() == 0)
		exe_name = ACE_TEXT("OOServer");

	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(exe_name.c_str()) == -1)
		return -1;

	// Set the creation flags
	u_long flags = 0;
#if defined (ACE_WIN32)
	flags |= CREATE_NEW_CONSOLE;
#endif
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to spawn server process\n")),-1);

	// Wait 1 second for the process to launch, if it takes more than 1 second its probably okay
	ACE_exitcode exitcode = 0;
	int ret = process.wait(ACE_Time_Value(1),&exitcode);
	if (ret==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed retrieve process exit code\n")),-1);

	if (ret!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Process exited with code %d.\n"),exitcode),-1);

    return 0;
}

int 
OOCore::Impl::Binding::find(const ACE_TCHAR* name, ACE_TString& value)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	ACE_TCHAR* pszType = 0;
	ACE_NS_WString w_val;
	int ret = m_context.resolve(name,w_val,pszType);
	ACE_OS::free(pszType);

	if (ret==0)
		value = ACE_TEXT_WCHAR_TO_TCHAR(w_val.c_str());

	return ret;
}

int 
OOCore::Impl::Binding::rebind(const ACE_TCHAR* name, const ACE_TCHAR* value)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	return m_context.rebind(name,value);
}

int 
OOCore::Impl::Binding::unbind(const ACE_TCHAR* name)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	return m_context.unbind(name);
}
