///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Win32.h"
#include "Mutex.h"
#include "TimeVal.h"
#include "SmartPtr.h"

#if defined(_WIN32)

namespace
{
	class Win32Thunk
	{
	public:
		Win32Thunk();

		typedef BOOL (__stdcall *pfn_InitOnceExecuteOnce)(INIT_ONCE* InitOnce, PINIT_ONCE_FN InitFn, void* Parameter, void** Context);
		pfn_InitOnceExecuteOnce m_InitOnceExecuteOnce;
		static BOOL __stdcall impl_InitOnceExecuteOnce(INIT_ONCE* InitOnce, PINIT_ONCE_FN InitFn, void* Parameter, void** Context);

		typedef void (__stdcall *pfn_InitializeSRWLock)(SRWLOCK* SRWLock);
		pfn_InitializeSRWLock m_InitializeSRWLock;
		static void __stdcall impl_InitializeSRWLock(SRWLOCK* SRWLock);
		
		typedef void (__stdcall *pfn_AcquireSRWLockShared)(SRWLOCK* SRWLock);
		pfn_AcquireSRWLockShared m_AcquireSRWLockShared;
		static void __stdcall impl_AcquireSRWLockShared(SRWLOCK* SRWLock);

		typedef void (__stdcall *pfn_AcquireSRWLockExclusive)(SRWLOCK* SRWLock);
		pfn_AcquireSRWLockExclusive m_AcquireSRWLockExclusive;
		static void __stdcall impl_AcquireSRWLockExclusive(SRWLOCK* SRWLock);

		typedef void (__stdcall *pfn_ReleaseSRWLockShared)(SRWLOCK* SRWLock);
		pfn_ReleaseSRWLockShared m_ReleaseSRWLockShared;
		static void __stdcall impl_ReleaseSRWLockShared(SRWLOCK* SRWLock);

		typedef void (__stdcall *pfn_ReleaseSRWLockExclusive)(SRWLOCK* SRWLock);
		pfn_ReleaseSRWLockExclusive m_ReleaseSRWLockExclusive;
		static void __stdcall impl_ReleaseSRWLockExclusive(SRWLOCK* SRWLock);

		typedef void (__stdcall *pfn_InitializeConditionVariable)(CONDITION_VARIABLE* ConditionVariable);
		pfn_InitializeConditionVariable m_InitializeConditionVariable;
		static void __stdcall impl_InitializeConditionVariable(CONDITION_VARIABLE* ConditionVariable);

		typedef BOOL (__stdcall *pfn_SleepConditionVariableCS)(CONDITION_VARIABLE* ConditionVariable, CRITICAL_SECTION* CriticalSection, DWORD dwMilliseconds);
		pfn_SleepConditionVariableCS m_SleepConditionVariableCS;
		
		typedef void (__stdcall *pfn_WakeConditionVariable)(CONDITION_VARIABLE* ConditionVariable);
		pfn_WakeConditionVariable m_WakeConditionVariable;
		static void __stdcall impl_WakeConditionVariable(CONDITION_VARIABLE* ConditionVariable);

		typedef void (__stdcall *pfn_WakeAllConditionVariable)(CONDITION_VARIABLE* ConditionVariable);
		pfn_WakeAllConditionVariable m_WakeAllConditionVariable;
		static void __stdcall impl_WakeAllConditionVariable(CONDITION_VARIABLE* ConditionVariable);

		typedef BOOL (__stdcall *pfn_BindIoCompletionCallback)(HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags);
		pfn_BindIoCompletionCallback m_BindIoCompletionCallback;
		
		HMODULE m_hKernel32;

	private:
		void init_low_frag_heap();
	};

	static Win32Thunk win32_thunk;
}

Win32Thunk::Win32Thunk() :
	m_hKernel32(0)
{
	m_hKernel32 = GetModuleHandleW(L"Kernel32.dll");
	if (!m_hKernel32)
		OOBase_CallCriticalFailure(GetLastError());

	m_InitOnceExecuteOnce = (pfn_InitOnceExecuteOnce)(GetProcAddress(m_hKernel32,"InitOnceExecuteOnce"));
	if (!m_InitOnceExecuteOnce)
		m_InitOnceExecuteOnce = impl_InitOnceExecuteOnce;

	m_InitializeSRWLock = (pfn_InitializeSRWLock)(GetProcAddress(m_hKernel32,"InitializeSRWLock"));
	m_AcquireSRWLockShared = (pfn_AcquireSRWLockShared)(GetProcAddress(m_hKernel32,"AcquireSRWLockShared"));
	m_AcquireSRWLockExclusive = (pfn_AcquireSRWLockExclusive)(GetProcAddress(m_hKernel32,"AcquireSRWLockExclusive"));
	m_ReleaseSRWLockShared = (pfn_ReleaseSRWLockShared)(GetProcAddress(m_hKernel32,"ReleaseSRWLockShared"));
	m_ReleaseSRWLockExclusive = (pfn_ReleaseSRWLockExclusive)(GetProcAddress(m_hKernel32,"ReleaseSRWLockExclusive"));
	
	if (!m_InitializeSRWLock ||
		!m_AcquireSRWLockShared ||
		!m_AcquireSRWLockExclusive ||
		!m_ReleaseSRWLockShared ||
		!m_ReleaseSRWLockExclusive)
	{
		m_InitializeSRWLock = impl_InitializeSRWLock;
		m_AcquireSRWLockShared = impl_AcquireSRWLockShared;
		m_AcquireSRWLockExclusive = impl_AcquireSRWLockExclusive;
		m_ReleaseSRWLockShared = impl_ReleaseSRWLockShared;
		m_ReleaseSRWLockExclusive = impl_ReleaseSRWLockExclusive;
	}

	m_InitializeConditionVariable = (pfn_InitializeConditionVariable)(GetProcAddress(m_hKernel32,"InitializeConditionVariable"));
	m_SleepConditionVariableCS = (pfn_SleepConditionVariableCS)(GetProcAddress(m_hKernel32,"SleepConditionVariableCS"));
	m_WakeConditionVariable = (pfn_WakeConditionVariable)(GetProcAddress(m_hKernel32,"WakeConditionVariable"));
	m_WakeAllConditionVariable = (pfn_WakeAllConditionVariable)(GetProcAddress(m_hKernel32,"WakeAllConditionVariable"));
	
	if (!m_InitializeConditionVariable ||
		!m_SleepConditionVariableCS ||
		!m_WakeConditionVariable ||
		!m_WakeAllConditionVariable)
	{
		m_InitializeConditionVariable = impl_InitializeConditionVariable;
		m_SleepConditionVariableCS = 0;
		m_WakeConditionVariable = impl_WakeConditionVariable;
		m_WakeAllConditionVariable = impl_WakeAllConditionVariable;
	}

	m_BindIoCompletionCallback = (pfn_BindIoCompletionCallback)(GetProcAddress(m_hKernel32,"BindIoCompletionCallback"));

	init_low_frag_heap();
}

void Win32Thunk::init_low_frag_heap()
{
#if (WINVER >= 0x0501)
	typedef BOOL (__stdcall *pfn_HeapSetInformation)(HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, void* HeapInformation, SIZE_T HeapInformationLength);
		
	pfn_HeapSetInformation pfn = (pfn_HeapSetInformation)(GetProcAddress(m_hKernel32,"HeapSetInformation"));
	if (pfn)
	{
		ULONG ulEnableLFH = 2;
		(*pfn)((HANDLE)_get_heap_handle(),HeapCompatibilityInformation,&ulEnableLFH, sizeof(ulEnableLFH));
	}
#endif
}

namespace 
{
	static HANDLE get_mutex(void* v)
	{
		wchar_t buf[256] = {0};
		wsprintfW(buf,L"OOBase__Win32Thunk__Once__Mutex__%lu__%p",GetCurrentProcessId(),v);
		HANDLE h = CreateMutexW(NULL,FALSE,buf);
		if (!h)
			OOBase_CallCriticalFailure(GetLastError());

		return h;
	}
}

BOOL Win32Thunk::impl_InitOnceExecuteOnce(INIT_ONCE* InitOnce, PINIT_ONCE_FN InitFn, void* Parameter, void** Context)
{
	OOBase::Win32::init_once_t* Once = reinterpret_cast<OOBase::Win32::init_once_t*>(InitOnce);

	LONG checked = InterlockedCompareExchange(&Once->check,-1,0);
	if (checked != 1)
	{
		// Get lock...
		OOBase::Win32::SmartHandle mutex(get_mutex(Once));

		do
		{
			if (WaitForSingleObject(mutex,INFINITE) != WAIT_OBJECT_0)
				OOBase_CallCriticalFailure(GetLastError());

			if (checked == 0)
			{
				// First in...
				try
				{
					if ((*InitFn)(InitOnce,Parameter,Context))
						InterlockedExchange(&Once->check,1);
					else
						InterlockedExchange(&Once->check,0);
				}
				catch (...)
				{
					InterlockedExchange(&Once->check,0);
					ReleaseMutex(mutex);
					return FALSE;
				}
			}

			checked = Once->check;
					
			// And release
			if (!ReleaseMutex(mutex))
				OOBase_CallCriticalFailure(GetLastError());

		} while (checked != 1);
	}
	
	return TRUE;
}

BOOL OOBase::Win32::InitOnceExecuteOnce(INIT_ONCE* InitOnce, PINIT_ONCE_FN InitFn, void* Parameter, void** Context)
{
#if (WINVER >= 0x0600)
	assert(sizeof(init_once_t) >= sizeof(INIT_ONCE));
#endif

	return (*win32_thunk.m_InitOnceExecuteOnce)(InitOnce,InitFn,Parameter,Context);
}

void Win32Thunk::impl_InitializeSRWLock(SRWLOCK* SRWLock)
{
	OOBase::Win32::rwmutex_t** mtx = reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock);

	OOBASE_NEW((*mtx),OOBase::Win32::rwmutex_t());
	if (!(*mtx))
		OOBase_OutOfMemory();
}

void OOBase::Win32::InitializeSRWLock(SRWLOCK* SRWLock)
{
#if (WINVER >= 0x0600)
	assert(sizeof(rwmutex_t*) >= sizeof(SRWLOCK));
#endif

	(*win32_thunk.m_InitializeSRWLock)(SRWLock);
}

void Win32Thunk::impl_AcquireSRWLockShared(SRWLOCK* SRWLock)
{
	(*reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock))->acquire_read();
}

void OOBase::Win32::AcquireSRWLockShared(SRWLOCK* SRWLock)
{
	(*win32_thunk.m_AcquireSRWLockShared)(SRWLock);
}

void Win32Thunk::impl_AcquireSRWLockExclusive(SRWLOCK* SRWLock)
{
	(*reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock))->acquire();
}

void OOBase::Win32::AcquireSRWLockExclusive(SRWLOCK* SRWLock)
{
	(*win32_thunk.m_AcquireSRWLockExclusive)(SRWLock);
}

void Win32Thunk::impl_ReleaseSRWLockShared(SRWLOCK* SRWLock)
{
	(*reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock))->release_read();
}

void OOBase::Win32::ReleaseSRWLockShared(SRWLOCK* SRWLock)
{
	(*win32_thunk.m_ReleaseSRWLockShared)(SRWLock);
}

void Win32Thunk::impl_ReleaseSRWLockExclusive(SRWLOCK* SRWLock)
{
	(*reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock))->release();
}

void OOBase::Win32::ReleaseSRWLockExclusive(SRWLOCK* SRWLock)
{
	(*win32_thunk.m_ReleaseSRWLockExclusive)(SRWLock);
}

void OOBase::Win32::DeleteSRWLock(SRWLOCK* SRWLock)
{
	if (win32_thunk.m_InitializeSRWLock == Win32Thunk::impl_InitializeSRWLock)
		delete (*reinterpret_cast<OOBase::Win32::rwmutex_t**>(SRWLock));
}

OOBase::Win32::rwmutex_t::rwmutex_t() :
	m_nReaders(-1),
	m_hReaderEvent(NULL),
	m_hEvent(NULL),
	m_hWriterMutex(NULL)
{
	m_hReaderEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
	if (!m_hReaderEvent)
		OOBase_CallCriticalFailure(GetLastError());

	m_hEvent = CreateEventW(NULL,FALSE,TRUE,NULL);
	if (!m_hEvent)
		OOBase_CallCriticalFailure(GetLastError());

	m_hWriterMutex = CreateMutexW(NULL,FALSE,NULL);
	if (!m_hWriterMutex)
		OOBase_CallCriticalFailure(GetLastError());
}

OOBase::Win32::rwmutex_t::~rwmutex_t() 
{ 
}

void OOBase::Win32::rwmutex_t::acquire()
{
	if (WaitForSingleObject(m_hWriterMutex, INFINITE) != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());

	if (WaitForSingleObject(m_hEvent, INFINITE) != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());
}

void OOBase::Win32::rwmutex_t::release()
{
	if (!SetEvent(m_hEvent))
		OOBase_CallCriticalFailure(GetLastError());

	if (!ReleaseMutex(m_hWriterMutex))
		OOBase_CallCriticalFailure(GetLastError());
}

void OOBase::Win32::rwmutex_t::acquire_read()
{
	if (InterlockedIncrement(&m_nReaders) == 0)
	{
		if (WaitForSingleObject(m_hEvent, INFINITE) != WAIT_OBJECT_0)
			OOBase_CallCriticalFailure(GetLastError());

		if (!SetEvent(m_hReaderEvent))
			OOBase_CallCriticalFailure(GetLastError());
	}

	if (WaitForSingleObject(m_hReaderEvent, INFINITE) != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());
}
  
void OOBase::Win32::rwmutex_t::release_read()
{
	if (InterlockedDecrement(&m_nReaders) < 0)
	{
		if (!ResetEvent(m_hReaderEvent))
			OOBase_CallCriticalFailure(GetLastError());

		if (!SetEvent(m_hEvent))
			OOBase_CallCriticalFailure(GetLastError());
	}
}

void Win32Thunk::impl_InitializeConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	OOBase::Win32::condition_variable_t** var = reinterpret_cast<OOBase::Win32::condition_variable_t**>(ConditionVariable);

	OOBASE_NEW((*var),OOBase::Win32::condition_variable_t());
	if (!(*var))
		OOBase_OutOfMemory();
}

void OOBase::Win32::InitializeConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
#if (WINVER >= 0x0600)
	assert(sizeof(condition_variable_t*) >= sizeof(CONDITION_VARIABLE));
#endif

	(*win32_thunk.m_InitializeConditionVariable)(ConditionVariable);
}

BOOL OOBase::Win32::SleepConditionVariable(CONDITION_VARIABLE* ConditionVariable, condition_mutex_t* Mutex, DWORD dwMilliseconds)
{
	if (win32_thunk.m_SleepConditionVariableCS == 0)
		return (*reinterpret_cast<OOBase::Win32::condition_variable_t**>(ConditionVariable))->wait(Mutex->m_mutex,dwMilliseconds) ? TRUE : FALSE;
	else
		return (*win32_thunk.m_SleepConditionVariableCS)(ConditionVariable,&Mutex->m_cs,dwMilliseconds);
}

void Win32Thunk::impl_WakeConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	(*reinterpret_cast<OOBase::Win32::condition_variable_t**>(ConditionVariable))->signal();
}

void OOBase::Win32::WakeConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	(*win32_thunk.m_WakeConditionVariable)(ConditionVariable);
}

void Win32Thunk::impl_WakeAllConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	(*reinterpret_cast<OOBase::Win32::condition_variable_t**>(ConditionVariable))->broadcast();
}

void OOBase::Win32::WakeAllConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	(*win32_thunk.m_WakeAllConditionVariable)(ConditionVariable);
}

void OOBase::Win32::DeleteConditionVariable(CONDITION_VARIABLE* ConditionVariable)
{
	if (win32_thunk.m_InitializeConditionVariable == Win32Thunk::impl_InitializeConditionVariable)
		delete (*reinterpret_cast<OOBase::Win32::condition_variable_t**>(ConditionVariable));
}

BOOL OOBase::Win32::BindIoCompletionCallback(HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags)
{
	return (*win32_thunk.m_BindIoCompletionCallback)(FileHandle,Function,Flags);
}

OOBase::Win32::condition_mutex_t::condition_mutex_t()
{
	if (win32_thunk.m_InitializeConditionVariable == Win32Thunk::impl_InitializeConditionVariable)
	{
		m_mutex = CreateMutexW(NULL,FALSE,NULL);
		if (!m_mutex)
			OOBase_CallCriticalFailure(GetLastError());
	}
	else if (!InitializeCriticalSectionAndSpinCount(&m_cs,4001))
		OOBase_CallCriticalFailure(GetLastError());
}

OOBase::Win32::condition_mutex_t::~condition_mutex_t()
{
	if (win32_thunk.m_InitializeConditionVariable != Win32Thunk::impl_InitializeConditionVariable)
		DeleteCriticalSection(&m_cs);
}

bool OOBase::Win32::condition_mutex_t::tryacquire()
{
	if (win32_thunk.m_InitializeConditionVariable == Win32Thunk::impl_InitializeConditionVariable)
	{
		DWORD dwWait = WaitForSingleObject(m_mutex,0);
		if (dwWait == WAIT_OBJECT_0)
			return true;
		else if (dwWait != WAIT_TIMEOUT)
			OOBase_CallCriticalFailure(GetLastError());

		return false;
	}
	else
		return (TryEnterCriticalSection(&m_cs) ? true : false);
}

void OOBase::Win32::condition_mutex_t::acquire()
{
	if (win32_thunk.m_InitializeConditionVariable == Win32Thunk::impl_InitializeConditionVariable)
	{
		DWORD dwWait = WaitForSingleObject(m_mutex,INFINITE);
		if (dwWait != WAIT_OBJECT_0)
			OOBase_CallCriticalFailure(GetLastError());
	}
	else
		EnterCriticalSection(&m_cs);
}

void OOBase::Win32::condition_mutex_t::release()
{
	if (win32_thunk.m_InitializeConditionVariable == Win32Thunk::impl_InitializeConditionVariable)
	{
		if (!ReleaseMutex(m_mutex))
			OOBase_CallCriticalFailure(GetLastError());
	}
	else
		LeaveCriticalSection(&m_cs);
}

OOBase::Win32::condition_variable_t::condition_variable_t() :
	m_waiters(0),
	m_broadcast(false),
	m_sema(NULL),
	m_waiters_done(NULL)
{
	if (!InitializeCriticalSectionAndSpinCount(&m_waiters_lock,4001))
		OOBase_CallCriticalFailure(GetLastError());

	m_sema = CreateSemaphoreW(NULL,       // no security
	                          0,          // initially 0
	                          0x7fffffff, // max count
	                          NULL);      // unnamed 
	if (!m_sema)
	{
		DWORD dwErr = GetLastError();
		DeleteCriticalSection(&m_waiters_lock);
		OOBase_CallCriticalFailure(dwErr);
	}

	m_waiters_done = CreateEventW(NULL,  // no security
	                              FALSE, // auto-reset
	                              FALSE, // non-signaled initially
	                              NULL); // unnamed
	if (!m_waiters_done)
	{
		DWORD dwErr = GetLastError();
		DeleteCriticalSection(&m_waiters_lock);
		OOBase_CallCriticalFailure(dwErr);
	}
}

OOBase::Win32::condition_variable_t::~condition_variable_t()
{
	DeleteCriticalSection(&m_waiters_lock);
}

bool OOBase::Win32::condition_variable_t::wait(HANDLE hMutex, DWORD dwMilliseconds)
{
	// Use a countdown as we do multiple waits
	timeval_t wait(dwMilliseconds / 1000, (dwMilliseconds % 1000) * 1000);
	Countdown countdown(&wait);

	// Avoid race conditions.
	EnterCriticalSection(&m_waiters_lock);
	if (m_broadcast)
	{
		// If there is a broadcast in progress, spin here...
		LeaveCriticalSection(&m_waiters_lock);
		for (;;)
		{
			EnterCriticalSection(&m_waiters_lock);
			if (!m_broadcast)
				break;

			LeaveCriticalSection(&m_waiters_lock);
		}
	}	
	++m_waiters;
	LeaveCriticalSection(&m_waiters_lock);

	if (dwMilliseconds != INFINITE)
		countdown.update();
		
	// This call atomically releases the mutex and waits on the
	// semaphore until <signal> or <broadcast>
	// are called by another thread.
	DWORD dwWait = SignalObjectAndWait(hMutex,m_sema,(dwMilliseconds != INFINITE ? wait.msec() : INFINITE),FALSE);
	
	// Reacquire lock to avoid race conditions.
	EnterCriticalSection(&m_waiters_lock);

	// We're no longer waiting...
	--m_waiters;

	// Check to see if we're the last waiter after <broadcast>.
	bool last_waiter = m_broadcast && (m_waiters == 0);

	LeaveCriticalSection(&m_waiters_lock);

	// Check if we timed out
	if (dwWait == WAIT_OBJECT_0)
	{
		countdown.update();
		
		// If we're the last waiter thread during this particular broadcast
		// then let all the other threads proceed.
		if (last_waiter)
		{
			// This call atomically signals the <m_waiters_done> event and waits until
			// it can acquire the <hMutex>.  This is required to ensure fairness. 
			dwWait = SignalObjectAndWait(m_waiters_done,hMutex,(dwMilliseconds != INFINITE ? wait.msec() : INFINITE),FALSE);
		}
		else
		{
			// Always regain the external mutex since that's the guarantee we
			// give to our callers. 
			dwWait = WaitForSingleObject(hMutex,(dwMilliseconds != INFINITE ? wait.msec() : INFINITE));
		}
	}
		
	// Return the correct code
	if (dwWait == WAIT_OBJECT_0)
		return true;
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	if (last_waiter)
	{
		if (!SetEvent(m_waiters_done))
			OOBase_CallCriticalFailure(GetLastError());
	}

	return false;
}

void OOBase::Win32::condition_variable_t::signal()
{
	EnterCriticalSection(&m_waiters_lock);
	bool have_waiters = m_waiters > 0;
	LeaveCriticalSection(&m_waiters_lock);

	// If there aren't any waiters, then this is a no-op.  
	if (have_waiters && !ReleaseSemaphore(m_sema,1,0))
		OOBase_CallCriticalFailure(GetLastError());
}

void OOBase::Win32::condition_variable_t::broadcast()
{
	// This is needed to ensure that <m_waiters> and <m_broadcast> are
	// consistent relative to each other.
	EnterCriticalSection(&m_waiters_lock);
	bool have_waiters = false;

	if (m_waiters > 0) 
	{
		// We are broadcasting, even if there is just one waiter...
		// Record that we are broadcasting, which helps optimize
		// <wait> for the non-broadcast case.
		m_broadcast = true;
		have_waiters = true;
	}

	if (have_waiters) 
	{
		// Wake up all the waiters atomically.
		LONG lPrev = 0;
		if (!ReleaseSemaphore(m_sema,m_waiters,&lPrev))
		{
			DWORD dwErr = GetLastError();
			LeaveCriticalSection(&m_waiters_lock);
			OOBase_CallCriticalFailure(dwErr);
		}

		LeaveCriticalSection(&m_waiters_lock);

		// Wait for all the awakened threads to acquire the counting semaphore. 
		DWORD dwWait = WaitForSingleObject(m_waiters_done,INFINITE);

		// This assignment is okay, even without the <m_waiters_lock> held 
		// because no other waiter threads can wake up to access it.
		m_broadcast = false;

		if (dwWait != WAIT_OBJECT_0)
			OOBase_CallCriticalFailure(GetLastError());
	}
	else
		LeaveCriticalSection(&m_waiters_lock);
}

std::string OOBase::Win32::FormatMessage(DWORD dwErr)
{
	std::string ret = "Unknown system error";

	HMODULE hNT = GetModuleHandleW(L"NTDLL.DLL");
	LPVOID lpBuf;
	if (::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_FROM_HMODULE |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		hNT,
		dwErr,
		0,
		(LPSTR)&lpBuf,
		0,	NULL))
	{
		OOBase::SmartPtr<void,OOBase::Win32::LocalAllocDestructor<void> > lpMsgBuf = lpBuf;
		ret = (LPCSTR)lpMsgBuf.value();
	}

	// Printf the code
	char szBuf[64];
	wsprintfA(szBuf,"(%#x) ",dwErr);
	
	return szBuf + ret;
}

#endif // _WIN32
