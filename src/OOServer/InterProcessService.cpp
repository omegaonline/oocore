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

#ifdef OMEGA_DEBUG
void AttachDebugger(pid_t pid);
#endif

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

	// Set the creation flags
	u_long flags = 0;
#if defined(OMEGA_WIN32)
	flags |= CREATE_NEW_CONSOLE;

#if defined(OMEGA_DEBUG)
	HANDLE hDebugEvent = NULL;
	if (IsDebuggerPresent())
	{
		hDebugEvent = CreateEventW(NULL,FALSE,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
		if (!hDebugEvent && GetLastError()==ERROR_ALREADY_EXISTS)
			hDebugEvent = OpenEventW(EVENT_ALL_ACCESS,FALSE,L"Global\\OOSERVER_DEBUG_MUTEX");
	}
#endif // OMEGA_DEBUG
#endif // OMEGA_WIN32

	options.creation_flags(flags);

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

Registry::IRegistryKey* User::InterProcessService::GetRegistry()
{
	if (!m_ptrReg)
	{
		// Double lock for speed
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		if (!m_ptrReg)
		{
			if (m_ptrOMUser)
			{
				// Create a proxy to the server interface
				IObject* pIPS = 0;
				m_ptrOMUser->CreateRemoteInstance(Remoting::OID_InterProcessService,OMEGA_UUIDOF(Remoting::IInterProcessService),0,pIPS);
				ObjectPtr<Remoting::IInterProcessService> ptrIPS;
				ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

				// Get the running object table
				m_ptrReg.Attach(ptrIPS->GetRegistry());
			}
			else
			{
				ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
				ptrKey->Init(m_pManager,L"",0);
				
				m_ptrReg = static_cast<Omega::Registry::IRegistryKey*>(ptrKey);
			}
		}
	}

	return m_ptrReg.AddRef();
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	if (!m_ptrROT)
	{
		// Double lock for speed
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		if (!m_ptrROT)
		{
			m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstancePtr();
			try
			{
				m_ptrROT->Init(m_ptrOMSB);
			}
			catch (...)
			{
				m_ptrROT.Release();
				throw;
			}
		}
	}

	return m_ptrROT.AddRef();
}

void User::InterProcessService::GetRegisteredObject(const guid_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	// Make sure we have a registry pointer...
	if (!m_ptrReg)
		GetRegistry()->Release();

	// And a ROT
	if (!m_ptrROT)
		GetRunningObjectTable()->Release();

	// Change this to use monikers
	void* TODO;

	// Try RunningObjectTable first
	ObjectPtr<IObject> ptrObject;
	ptrObject.Attach(m_ptrROT->GetObject(oid));
	if (ptrObject)
	{
		pObject = ptrObject->QueryInterface(iid);
		if (!pObject)
			throw INoInterfaceException::Create(iid);
		return;
	}
	
	if (!(flags & Activation::DontLaunch))
	{
		// Lookup OID
		ObjectPtr<Omega::Registry::IRegistryKey> ptrOidsKey = m_ptrReg.OpenSubKey(L"Objects\\OIDs");
		if (ptrOidsKey->IsSubKey(oid.ToString()))
		{
			ObjectPtr<Omega::Registry::IRegistryKey> ptrOidKey = ptrOidsKey.OpenSubKey(oid.ToString());
			
			// Find the name of the executeable to run
			ObjectPtr<Omega::Registry::IRegistryKey> ptrServer = m_ptrReg.OpenSubKey(L"Applications\\" + ptrOidKey->GetStringValue(L"Application"));
			string_t strProcess = ptrServer->GetStringValue(L"Activation");

			// Check for 2 requests to activate the same process...
			void* TODO;

			// The timeout needs to be related to the request timeout...
			ACE_Time_Value deadline = ACE_Time_Value::max_time;
			ObjectPtr<Remoting::ICallContext> ptrCC;
			ptrCC.Attach(Remoting::GetCallContext());
			if (ptrCC)
			{
				uint64_t secs = 0;
				int32_t usecs = 0;
				ptrCC->Deadline(secs,usecs);
				deadline = ACE_Time_Value(secs,usecs);
			}			
			
			// Launch the executable
			ACE_Process process;
			ExecProcess(process,strProcess);

			// Wait for the process to start and register its parts...
			ACE_Time_Value wait;
			ACE_Time_Value now = ACE_OS::gettimeofday();
			if (deadline == ACE_Time_Value::max_time)
				wait = ACE_Time_Value(15);
			else if (deadline <= now)
				wait = ACE_Time_Value::zero;
			else
				wait = deadline - now;

			ACE_Countdown_Time timeout(&wait);
			do
			{
				ObjectPtr<IObject> ptrObject;
				ptrObject.Attach(m_ptrROT->GetObject(oid));
				if (ptrObject)
				{
					pObject = ptrObject->QueryInterface(iid);
					if (!pObject)
						throw INoInterfaceException::Create(iid);
					break;
				}

				// Check if the process is still running...
				if (!process.running())
					break;

				// Sleep for a brief moment - it will take a moment for the process to start
				ACE_OS::sleep(ACE_Time_Value(0,100));

				// Update our countdown
				timeout.update();

			} while (wait != ACE_Time_Value::zero);
		}
	}
}
