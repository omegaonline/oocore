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

#include "Mutex.h"

#if defined(_WIN32)

OOBase::SpinLock::SpinLock(unsigned int max_spin)
{
	InitializeCriticalSectionAndSpinCount(&m_cs,max_spin);
}

OOBase::SpinLock::~SpinLock()
{
	DeleteCriticalSection(&m_cs);
}

void OOBase::SpinLock::acquire()
{
	EnterCriticalSection(&m_cs);
}

bool OOBase::SpinLock::tryacquire()
{
	return (TryEnterCriticalSection(&m_cs) ? true : false);
}

void OOBase::SpinLock::release()
{
	LeaveCriticalSection(&m_cs);
}

OOBase::Mutex::Mutex() :
	m_mutex(NULL)
{
	m_mutex = CreateMutexW(NULL,FALSE,NULL);
	if (!m_mutex)
		OOBase_CallCriticalFailure(GetLastError());
}

OOBase::Mutex::~Mutex()
{
}

bool OOBase::Mutex::tryacquire()
{
	DWORD dwWait = WaitForSingleObject(m_mutex,0);
	if (dwWait == WAIT_OBJECT_0)
		return true;
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

bool OOBase::Mutex::acquire(const timeval_t* wait)
{
	DWORD dwWait = WaitForSingleObject(m_mutex,(wait ? wait->msec() : INFINITE));
	if (dwWait == WAIT_OBJECT_0)
		return true;
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

void OOBase::Mutex::release()
{
	if (m_mutex && !ReleaseMutex(m_mutex))
		OOBase_CallCriticalFailure(GetLastError());
}

OOBase::RWMutex::RWMutex()
{
	Win32::InitializeSRWLock(&m_lock);
}

OOBase::RWMutex::~RWMutex() 
{ 
	Win32::DeleteSRWLock(&m_lock);
}

void OOBase::RWMutex::acquire()
{
	Win32::AcquireSRWLockExclusive(&m_lock);
}

void OOBase::RWMutex::release()
{
	Win32::ReleaseSRWLockExclusive(&m_lock);
}

void OOBase::RWMutex::acquire_read()
{
	Win32::AcquireSRWLockShared(&m_lock);
}
  
void OOBase::RWMutex::release_read()
{
	Win32::ReleaseSRWLockShared(&m_lock);
}

#else

#error Fix me!

#endif
