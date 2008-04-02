///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

#include "OOServer.h"

#include "./InterProcessService.h"
#include "./UserManager.h"
#include "./NetTcp.h"
#include "./NetHttp.h"

using namespace Omega;
using namespace OTL;

ACE_WString User::ShellParse(const wchar_t* pszFile)
{
	ACE_WString strRet = pszFile;

#if defined(OMEGA_WIN32)

#if defined(__MINGW32__)
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
#endif
		
	const wchar_t* pszExt = PathFindExtensionW(pszFile);
	if (pszExt)
	{
		DWORD dwLen = 1024;
		wchar_t szBuf[1024];	
		HRESULT hRes = AssocQueryStringW(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL,ASSOCSTR_COMMAND,pszExt,NULL,szBuf,&dwLen);
		if (hRes == S_OK)
			strRet = szBuf;
		else if (hRes == E_POINTER)
		{
			wchar_t* pszBuf = new wchar_t[dwLen+1];
			hRes = AssocQueryStringW(ASSOCF_NOTRUNCATE | ASSOCF_REMAPRUNDLL,ASSOCSTR_COMMAND,pszExt,NULL,pszBuf,&dwLen);
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
	options.handle_inheritence(0);

	// Do a ShellExecute style lookup for the actual thing to call..
	ACE_WString strActualName = ShellParse(strExeName.c_str());

	if (options.command_line(strActualName.c_str()) == -1)
		OOSERVER_THROW_LASTERROR();

#if defined(OMEGA_WIN32) && defined(OMEGA_DEBUG)
	HANDLE hDebugEvent = NULL;
	if (IsDebuggerPresent())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		if (!hDebugEvent && GetLastError()==ERROR_ALREADY_EXISTS)
			hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
	}
#endif // OMEGA_DEBUG && OMEGA_WIN32

	// Spawn the process
	if (process.spawn(options)==ACE_INVALID_PID)
		OOSERVER_THROW_LASTERROR();

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
		ptrOMSB->CreateRemoteInstance(Remoting::OID_InterProcessService,OMEGA_UUIDOF(Remoting::IInterProcessService),0,pIPS);
		m_ptrSBIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));
	}

	if (ptrOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOMUser->CreateRemoteInstance(Remoting::OID_InterProcessService,OMEGA_UUIDOF(Remoting::IInterProcessService),0,pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrReg.Attach(ptrIPS->GetRegistry());
	}
	else
	{
		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
		ptrKey->Init(m_pManager,L"",0);
		
		m_ptrReg = static_cast<Omega::Registry::IRegistryKey*>(ptrKey);
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

Registry::IRegistryKey* User::InterProcessService::GetRegistry()
{
	return m_ptrReg.AddRef();
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	return m_ptrROT.AddRef();
}

bool_t User::InterProcessService::ExecProcess(const string_t& strProcess, bool_t bPublic)
{
	if (bPublic && m_ptrSBIPS)
		return m_ptrSBIPS->ExecProcess(strProcess,false);
	
	ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Null_Mutex> ptrProcess;

	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	std::map<string_t,ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Null_Mutex> >::iterator i = m_mapInProgress.find(strProcess);
	if (i != m_mapInProgress.end())
	{
		ptrProcess = i->second;
		if (!ptrProcess->running())
		{
			m_mapInProgress.erase(strProcess);
			ptrProcess.release();
		}
	}

	if (ptrProcess.null())
	{
		// Create a new process
		OMEGA_NEW(ptrProcess,ACE_Process());
		
		// Start it
		User::ExecProcess(*ptrProcess,strProcess);

		m_mapInProgress.insert(std::map<string_t,ACE_Refcounted_Auto_Ptr<ACE_Process,ACE_Null_Mutex> >::value_type(strProcess,ptrProcess));
	}
	
	if (!ptrProcess->running())
	{
		m_mapInProgress.erase(strProcess);

		return false;
	}
	
	return true;
}

IO::IStream* User::InterProcessService::OpenStream(const string_t& strEndPoint, IO::IAsyncStreamCallback* pCallback)
{
	// First try to determine the protocol...
	size_t pos = strEndPoint.Find(L"://");
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndPoint.Left(pos).ToLower();
	
	guid_t oid = guid_t::Null();
	ObjectPtr<Omega::Registry::IRegistryKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"ServerHandlerOID"))
			oid = guid_t::FromString(ptrKey->GetStringValue(L"ServerHandlerOID"));
	}
	
	if (oid == guid_t::Null())
	{
		ptrKey = ObjectPtr<Omega::Registry::IRegistryKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"ServerHandlerOID"))
				oid = guid_t::FromString(ptrKey->GetStringValue(L"ServerHandlerOID"));
		}
	}
	
	if (oid == guid_t::Null())
	{
		if (strProtocol == L"tcp")
			oid = OID_TcpProtocolHandler;
		else if (strProtocol == L"http")
			oid = OID_HttpProtocolHandler;
		else
			OMEGA_THROW(L"No handler for protocol " + strProtocol);		
	}
	
	// Create the handler...
	ObjectPtr<IO::IProtocolHandler> ptrHandler(oid);

	// Open the stream...
	return ptrHandler->OpenStream(strEndPoint,pCallback);
}
