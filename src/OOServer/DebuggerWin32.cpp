///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#include "./OOServer_Root.h"

///////////////////////////////////////////////////////////////////////////////////
//
// This module provides hook functions that attempt to attach to a debugger...
//
///////////////////////////////////////////////////////////////////////////////////

#if defined(OMEGA_DEBUG)

#if defined(_MSC_VER) && _MSC_VER >= 1300
#if _MSC_VER == 1300
#import "C:\Program Files\Common Files\Microsoft Shared\MSEnv\dte.olb" raw_interfaces_only named_guids
using namespace EnvDTE;
#elif _MSC_VER >= 1400
#import "C:\Program Files\Common Files\Microsoft Shared\MSEnv\dte80a.olb" raw_interfaces_only named_guids rename("GetObject","dte_GetObject") rename("SearchPath","dte_SearchPath")
using namespace EnvDTE;
#endif
#if _MSC_VER >= 1400
#import "C:\Program Files\Common Files\Microsoft Shared\MSEnv\dte80.olb" raw_interfaces_only named_guids
using namespace EnvDTE80;
#endif

static bool AttachVSDebugger(DWORD our_pid)
{
	bool bRet = false;
	HRESULT hr = CoInitialize(NULL);
	if SUCCEEDED(hr)
	{
		IUnknownPtr pUnk;
	#if _MSC_VER >= 1400
		pUnk.GetActiveObject("VisualStudio.DTE.8.0");
	#elif _MSC_VER == 1300
		pUnk.GetActiveObject("VisualStudio.DTE.7.1");
	#endif
		if (pUnk != NULL)
		{
			_DTEPtr pDTE = pUnk;
			if (pDTE)
			{
				DebuggerPtr pDebugger;
				if (SUCCEEDED(pDTE->get_Debugger(&pDebugger)) && pDebugger != NULL)
				{
					ProcessesPtr pProcesses;
					if (SUCCEEDED(pDebugger->get_LocalProcesses(&pProcesses)) && pProcesses != NULL)
					{
						long lCount = 0;
						pProcesses->get_Count(&lCount);
						for (long i=1;i<=lCount;++i)
						{
							ProcessPtr pProcess;
							if (SUCCEEDED(pProcesses->Item(variant_t(i),&pProcess)) && pProcess != NULL)
							{
								long pid = 0;
								if (SUCCEEDED(pProcess->get_ProcessID(&pid)))
								{
									if (static_cast<DWORD>(pid) == our_pid)
									{
										if SUCCEEDED(pProcess->Attach())
											bRet = true;

										break;
									}
								}
							}
						}
					}
				}
			}

			pUnk.Release();
		}
		CoUninitialize();
	}

	return bRet;
}
#endif

static void PromptForDebugger(DWORD pid)
{
	wchar_t szBuf[256];
	ACE_OS::snprintf(szBuf,256,L"Attach the debugger to process id %lu now if you want!",pid);
	MessageBoxW(NULL,szBuf,L"Break",MB_ICONEXCLAMATION | MB_OK | MB_SERVICE_NOTIFICATION);
}

void AttachDebugger(pid_t pid)
{
#if defined(_MSC_VER) && _MSC_VER >= 1300
	if (AttachVSDebugger(pid))
		return;
#endif

	PromptForDebugger(pid);
}

#endif // OMEGA_DEBUG
