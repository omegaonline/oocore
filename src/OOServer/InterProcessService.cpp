///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "./OOServer_User.h"
#include "./InterProcessService.h"
#include "./UserManager.h"
#include "./NetTcp.h"
#include "./NetHttp.h"

using namespace Omega;
using namespace OTL;

namespace User
{
	void ExecProcess(ACE_Process& process, const Omega::string_t& strExeName);
	ACE_WString ShellParse(const wchar_t* pszFile);
}

ACE_WString User::ShellParse(const wchar_t* pszFile)
{
	ACE_WString strRet = pszFile;

#if defined(OMEGA_WIN32)

/*#if defined(__MINGW32__)
	// These are missing from mingGW
	enum {
		ASSOCF_INIT_NOREMAPCLSID           = 0x00000001,  //  do not remap clsids to progids
		ASSOCF_INIT_BYEXENAME              = 0x00000002,  //  executable is being passed in
		ASSOCF_OPEN_BYEXENAME              = 0x00000002,  //  executable is being passed in
		ASSOCF_INIT_DEFAULTTOSTAR          = 0x00000004,  //  treat "*" as the BaseClass
		ASSOCF_INIT_DEFAULTTOFOLDER        = 0x00000008,  //  treat "Folder" as the BaseClass
		ASSOCF_NOUSERSETTINGS              = 0x00000010,  //  dont use HKCU
		ASSOCF_NOTRUNCATE                  = 0x00000020,  //  dont truncate the return string
		ASSOCF_VERIFY                      = 0x00000040,  //  verify data is accurate (DISK HITS)
		ASSOCF_REMAPRUNDLL                 = 0x00000080,  //  actually gets info about rundlls target if applicable
		ASSOCF_NOFIXUPS                    = 0x00000100,  //  attempt to fix errors if found
		ASSOCF_IGNOREBASECLASS             = 0x00000200,  //  dont recurse into the baseclass
	};
#endif*/

	const wchar_t* pszExt = PathFindExtensionW(pszFile);
	if (pszExt && ACE_OS::strcasecmp(pszExt,L".exe")!=0)
	{
		DWORD dwLen = 1024;
		wchar_t szBuf[1024];
		ASSOCF flags = (ASSOCF)(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL);
		HRESULT hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,szBuf,&dwLen);
		if (hRes == S_OK)
			strRet = szBuf;
		else if (hRes == E_POINTER)
		{
			wchar_t* pszBuf = new wchar_t[dwLen+1];
			hRes = AssocQueryStringW(flags,ASSOCSTR_COMMAND,pszExt,NULL,pszBuf,&dwLen);
			if (hRes==S_OK)
				strRet = pszBuf;

			delete [] pszBuf;
		}

		if (hRes == S_OK)
		{
			LPVOID lpBuffer = 0;
			if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
				strRet.c_str(),0,0,(LPWSTR)&lpBuffer,0,(va_list*)&pszFile))
			{
				strRet = (LPWSTR)lpBuffer;
				LocalFree(lpBuffer);
			}
		}
	}

#endif

	return strRet;
}

void User::ExecProcess(ACE_Process& process, const string_t& strExeName)
{
	// Set the process options
	ACE_Process_Options options(0);
	options.avoid_zombies(0);

#if OMEGA_ACE_VERSION_CURRENT() < OMEGA_ACE_VERSION(5,6,7)
	options.handle_inheritence(0);
#else
	options.handle_inheritance(0);
#endif

	// Do a ShellExecute style lookup for the actual thing to call..
	ACE_WString strActualName = ShellParse(strExeName.c_str());

	if (options.command_line(strActualName.c_str()) == -1)
		OMEGA_THROW(ACE_OS::last_error());

#if defined(OMEGA_WIN32) && defined(OMEGA_DEBUG)
	HANDLE hDebugEvent = NULL;
	if (IsDebuggerPresent())
	{
		options.creation_flags(CREATE_NEW_CONSOLE);

		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		if (!hDebugEvent && GetLastError()==ERROR_ALREADY_EXISTS)
			hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
	}
#endif // OMEGA_DEBUG && OMEGA_WIN32

	// Spawn the process
	if (process.spawn(options)==ACE_INVALID_PID)
		OMEGA_THROW(ACE_OS::last_error());

#if defined(OMEGA_WIN32) && defined(OMEGA_DEBUG)
	if (hDebugEvent)
	{
		AttachDebugger(process.getpid());

		SetEvent(hDebugEvent);
		CloseHandle(hDebugEvent);
	}
#endif
}

void User::InterProcessService::Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMSB, OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMUser, Manager* pManager)
{
	m_pManager = pManager;

	// Create a proxy to the server interface
	if (ptrOMSB)
	{
		IObject* pIPS = 0;
		ptrOMSB->GetRemoteInstance(System::OID_InterProcessService.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(System::IInterProcessService),pIPS);
		m_ptrSBIPS.Attach(static_cast<System::IInterProcessService*>(pIPS));
	}

	if (ptrOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOMUser->GetRemoteInstance(System::OID_InterProcessService.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(System::IInterProcessService),pIPS);
		ObjectPtr<System::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<System::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrReg.Attach(ptrIPS->GetRegistry());
	}
	else
	{
		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
		ptrKey->Init(m_pManager,L"",0);

		m_ptrReg = static_cast<Omega::Registry::IKey*>(ptrKey);
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstancePtr();
	try
	{
		m_ptrROT->Init(ptrOMSB);
	}
	catch (...)
	{
		m_ptrROT.Release();
		throw;
	}
}

Registry::IKey* User::InterProcessService::GetRegistry()
{
	return m_ptrReg.AddRef();
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	return m_ptrROT.AddRef();
}

void User::InterProcessService::GetObject(const string_t& strProcess, bool_t bPublic, const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	if (bPublic && m_ptrSBIPS)
		return m_ptrSBIPS->GetObject(strProcess,false,oid,iid,pObject);

	// The timeout needs to be related to the request timeout...
	ACE_Time_Value wait(15);
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = ACE_Time_Value(msecs / 1000,(msecs % 1000) * 1000);
	}

	ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Thread_Mutex> ptrProcess;

	bool bStarted = false;
	do
	{
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		std::map<string_t,ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Thread_Mutex> >::iterator i = m_mapInProgress.find(strProcess);
		if (i != m_mapInProgress.end())
		{
			ptrProcess = i->second;

			if (!ptrProcess->running())
			{
				m_mapInProgress.erase(strProcess);
				ptrProcess.reset(0);
			}
		}

		if (ptrProcess.null())
		{
			// Create a new process
			ACE_Process* pProcess = 0;
			OMEGA_NEW(pProcess,ACE_Process());
			ptrProcess.reset(pProcess);

			// Start it
			User::ExecProcess(*ptrProcess,strProcess);

			m_mapInProgress.insert(std::map<string_t,ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Thread_Mutex> >::value_type(strProcess,ptrProcess));

			bStarted = true;
		}

		guard.release();

		// Wait for the process to start and register its parts...
		ACE_Countdown_Time timeout(&wait);
		while (wait != ACE_Time_Value::zero)
		{
			ObjectPtr<IObject> ptrObject;
			ptrObject.Attach(m_ptrROT->GetObject(oid));
			if (ptrObject)
			{
				pObject = ptrObject->QueryInterface(iid);
				if (!pObject)
					throw INoInterfaceException::Create(iid);
				return;
			}

			// Check the process is still alive
			ACE_exitcode ec = 0;
			if (ptrProcess->wait(ACE_Time_Value(0,100000),&ec) != 0)
				break;

			// Update our countdown
			timeout.update();
		}

		if (wait == ACE_Time_Value::zero)
			OMEGA_THROW(ETIMEDOUT);

		// Remove from the map
		guard.acquire();
		m_mapInProgress.erase(strProcess);
		ptrProcess.reset(0);
		guard.release();

	} while (!bStarted);

	OMEGA_THROW(ENOENT);
}

IO::IStream* User::InterProcessService::OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndpoint.Left(pos).ToLower();

	string_t strHandler;
	ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"Handler"))
			strHandler = ptrKey->GetStringValue(L"Handler");
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"Handler"))
				strHandler = ptrKey->GetStringValue(L"Handler");
		}
	}

	guid_t oid = guid_t::Null();
	if (!strHandler.IsEmpty())
	{
		oid = guid_t::FromString(strHandler);
		if (oid == guid_t::Null())
			oid = Activation::NameToOid(strHandler);
	}

	if (oid == guid_t::Null())
	{
		// We have some built-ins
		if (strProtocol == L"tcp")
			oid = OID_TcpProtocolHandler;
		else if (strProtocol == L"http" || strProtocol == L"https")
			oid = OID_HttpProtocolHandler;
		else
			OMEGA_THROW(L"No handler for protocol " + strProtocol);
	}

	// Create the handler...
	ObjectPtr<Net::IProtocolHandler> ptrHandler(oid);

	// Open the stream...
	return ptrHandler->OpenStream(strEndpoint,pNotify);
}

bool_t User::InterProcessService::HandleRequest(uint32_t timeout)
{
	ACE_Time_Value wait(timeout/1000,(timeout % 1000) * 1000);
	wait += ACE_OS::gettimeofday();

	ACE_Time_Value* wait2 = &wait;
	if (timeout == (uint32_t)0)
		wait2 = 0;

	int ret = m_pManager->pump_requests(wait2,true);
	if (ret == -1)
		OMEGA_THROW(ACE_OS::last_error());
	else
		return (ret == 0 ? false : true);
}

Remoting::IChannel* User::InterProcessService::OpenRemoteChannel(const string_t& strEndpoint)
{
	return Manager::open_remote_channel(strEndpoint);
}

Remoting::IChannelSink* User::InterProcessService::OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return Manager::open_server_sink(message_oid,pSink);
}
