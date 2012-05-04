///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

/////////////////////////////////////////////////////////////
//
//  ***** THIS IS A SECURE MODULE *****
//
//  It can be run as Administrator/setuid root
//
//  Therefore it needs to be SAFE AS HOUSES!
//
//  Do not include anything unnecessary
//
/////////////////////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/String.h>

#if defined(_WIN32)

///////////////////////////////////////////////////////////////////////////////////
//
// This module provides hook functions that attempt to attach to a debugger...
//
///////////////////////////////////////////////////////////////////////////////////

#if !defined(NDEBUG) && defined(_MSC_VER)

// #import the guts of visual studio
#if _MSC_VER == 1310

//The following #import imports EnvDTE based on its LIBID.
#import "libid:2df8d04c-5bfa-101b-bde5-00aa0044de52" raw_interfaces_only rename("RGB","dte_RGB") rename("DocumentProperties","dte_DocumentProperties")
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("7.0") rename("GetObject","dte_GetObject") rename("SearchPath","dte_SearchPath") rename("FindText","dte_FindText") rename("ReplaceText","dte_ReplaceText")

#elif _MSC_VER > 1310

//The following #import imports EnvDTE based on its LIBID.
#import "libid:80cc9f66-e7d8-4ddd-85b6-d9e6cd0e93e2" version("8.0") rename("GetObject","dte_GetObject") rename("SearchPath","dte_SearchPath") rename("FindText","dte_FindText") rename("ReplaceText","dte_ReplaceText")

#endif

#if _MSC_VER == 1310
#define DTE_VER "7.1"
#elif _MSC_VER == 1400
#define DTE_VER "8.0"
#elif _MSC_VER == 1500
#define DTE_VER "9.0"
#elif _MSC_VER == 1600
#define DTE_VER "10.0"
#else

// MSVC 11 isn't out as I write this...
#error Fix me for the new release of Visual Studio!

#endif

namespace
{
	class MyMessageFilter : public IMessageFilter
	{
	public:
		MyMessageFilter() : m_refcount(1)
		{}

		ULONG STDMETHODCALLTYPE AddRef()
		{
			return ++m_refcount;
		}

		ULONG STDMETHODCALLTYPE Release()
		{
			ULONG ret = --m_refcount;
			if (!ret)
				delete this;
			return ret;
		}

		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject)
		{
			if (iid == IID_IUnknown ||
					iid == IID_IMessageFilter)
			{
				*ppvObject = this;
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		DWORD STDMETHODCALLTYPE HandleInComingCall(DWORD /*dwCallType*/, HTASK /*threadIDCaller*/, DWORD /*dwTickCount*/, LPINTERFACEINFO /*lpInterfaceInfo*/)
		{
			return SERVERCALL_ISHANDLED;
		}

		DWORD STDMETHODCALLTYPE RetryRejectedCall(HTASK /*threadIDCallee*/, DWORD /*dwTickCount*/, DWORD dwRejectType)
		{
			if (dwRejectType == SERVERCALL_RETRYLATER)
			{
				// Retry the thread call immediately if return >=0 &
				// <100.
				return 99;
			}
			// Too busy; cancel call.
			return (DWORD)-1;
		}

		DWORD STDMETHODCALLTYPE MessagePending(HTASK /*threadIDCallee*/, DWORD /*dwTickCount*/, DWORD /*dwPendingType*/)
		{
			return PENDINGMSG_WAITDEFPROCESS;
		}

	private:
		ULONG m_refcount;
	};

	bool AttachVSDebugger(DWORD our_pid)
	{
		bool bRet = false;
		HRESULT hr = CoInitialize(NULL);
		if FAILED(hr)
			return false;

		try
		{
			MyMessageFilter* pFilter = new (std::nothrow) MyMessageFilter();
			if (!pFilter)
				return false;

			IMessageFilter* pPrev = 0;
			hr = CoRegisterMessageFilter(pFilter,&pPrev);
			if FAILED(hr)
				throw _com_error(hr);
			pFilter->Release();
			if (pPrev)
				pPrev->Release();

			IUnknownPtr ptrUnk;
			ptrUnk.GetActiveObject("VisualStudio.DTE." DTE_VER);
			if (ptrUnk != NULL)
			{
				EnvDTE::_DTEPtr ptrDTE = ptrUnk;

				EnvDTE::ProcessesPtr ptrProcesses = ptrDTE->Debugger->LocalProcesses;
				for (long i = 1; i <= ptrProcesses->Count; ++i)
				{
					EnvDTE::ProcessPtr ptrProcess = ptrProcesses->Item(i);
					if (ptrProcess->ProcessID == static_cast<long>(our_pid))
					{
						ptrProcess->Attach();
						bRet = true;
						break;
					}
				}
			}
		}
		catch (_com_error&)
		{}

		CoUninitialize();

		return bRet;
	}
}

#endif

namespace
{
	void PromptForDebugger(DWORD pid)
	{
		OOBase::LocalString str;
		if (str.printf("Attach the debugger to process id %ld now if you want!",pid) == 0)
			MessageBoxA(NULL,str.c_str(),"Break",MB_ICONEXCLAMATION | MB_OK | MB_SERVICE_NOTIFICATION);
	}
}

void AttachDebugger(DWORD pid)
{
#if !defined(NDEBUG) && defined(_MSC_VER)
	if (!AttachVSDebugger(pid))
#endif
		PromptForDebugger(pid);
}

#endif // _WIN32
