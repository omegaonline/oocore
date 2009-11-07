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
	if (!ReleaseMutex(m_mutex))
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

#if defined(HAVE_FUTEX_H)

OOBase::SpinLock::SpinLock(unsigned int max_spin)
{
	#error Fix me!
}

#endif

#if defined(HAVE_PTHREAD)

OOBase::Mutex::Mutex()
{
	pthread_mutexattr_t attr;
	int err = pthread_mutexattr_init(&attr);
	if (!err)
	{
		err = pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
		if (!err)
			err = pthread_mutex_init(&m_mutex,&attr);

		pthread_mutexattr_destroy(&attr);
	}

	if (err)
		OOBase_CallCriticalFailure(err);
}

OOBase::Mutex::~Mutex()
{
	pthread_mutex_destroy(&m_mutex);
}

bool OOBase::Mutex::tryacquire()
{
	int err = pthread_mutex_trylock(&m_mutex);
	if (err == 0)
		return true;

	if (err != EBUSY)
		OOBase_CallCriticalFailure(err);

	return false;
}

bool OOBase::Mutex::acquire(const timeval_t* wait)
{
	int err = 0;
	if (!wait)
	{
		err = pthread_mutex_lock(&m_mutex);
		if (!err)
			return true;
	}
	else
	{
		timespec ts;
		OOBase::timeval_t now = OOBase::gettimeofday();
		now += *wait;
		ts.tv_sec = now.tv_sec();
		ts.tv_nsec = now.tv_usec() * 1000;

		int err = pthread_mutex_timedlock(&m_mutex,&ts);
		if (err == 0)
			return true;
		else if (err == ETIMEDOUT)
			return false;
	}

	OOBase_CallCriticalFailure(err);
	return false;
}

void OOBase::Mutex::release()
{
	int err = pthread_mutex_unlock(&m_mutex);
	if (err != 0)
		OOBase_CallCriticalFailure(err);
}

OOBase::RWMutex::RWMutex()
{
	int err = pthread_rwlock_init(&m_mutex,NULL);
	if (err)
		OOBase_CallCriticalFailure(err);
}

OOBase::RWMutex::~RWMutex()
{
	pthread_rwlock_destroy(&m_mutex);
}

void OOBase::RWMutex::acquire()
{
	int err = pthread_rwlock_wrlock(&m_mutex);
	if (err)
		OOBase_CallCriticalFailure(err);
}

void OOBase::RWMutex::release()
{
	int err = pthread_rwlock_unlock(&m_mutex);
	if (err != 0)
		OOBase_CallCriticalFailure(err);
}

void OOBase::RWMutex::acquire_read()
{
	int err = pthread_rwlock_rdlock(&m_mutex);
	if (err)
		OOBase_CallCriticalFailure(err);
}

void OOBase::RWMutex::release_read()
{
	release();
}

#else

#error Fix me!

#endif

#endif
