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

#include "Condition.h"

#if defined(_WIN32)

OOBase::Condition::Condition()
{
	Win32::InitializeConditionVariable(&m_var);
}

OOBase::Condition::~Condition()
{
	Win32::DeleteConditionVariable(&m_var);
}

bool OOBase::Condition::wait(Condition::Mutex& mutex, const timeval_t* wait)
{
	return (Win32::SleepConditionVariable(&m_var,&mutex,wait ? wait->msec() : INFINITE) != FALSE);
}

void OOBase::Condition::signal()
{
	Win32::WakeConditionVariable(&m_var);
}

void OOBase::Condition::broadcast()
{
	Win32::WakeAllConditionVariable(&m_var);
}

#elif defined(HAVE_PTHREAD)

OOBase::Condition::Condition()
{
	int err = pthread_cond_init(&m_var,NULL);
	if (err)
		OOBase_CallCriticalFailure(err);
}

OOBase::Condition::~Condition()
{
	pthread_cond_destroy(&m_var);
}

bool OOBase::Condition::wait(Condition::Mutex& mutex, const timeval_t* wait)
{
	int err = 0;
	if (!wait)
		err = pthread_cond_wait(&m_var,&mutex.m_mutex);
	else
	{
		timespec wt;
		timeval_t now = OOBase::gettimeofday();
		now += *wait;
		wt.tv_sec = now.tv_sec;
		wt.tv_nsec = now.tv_usec * 1000;

    	err = pthread_cond_timedwait(&m_var,&mutex.m_mutex,&wt);
	}

	if (err == 0)
		return true;

	if (err != ETIMEDOUT)
		OOBase_CallCriticalFailure(err);

	return false;
}

void OOBase::Condition::signal()
{
	pthread_cond_signal(&m_var);
}

void OOBase::Condition::broadcast()
{
	pthread_cond_broadcast(&m_var);
}

#else

#error Fix me!

#endif
