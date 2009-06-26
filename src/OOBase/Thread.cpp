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

#include "Thread.h"

#if defined(_WIN32)

#include <process.h>

#include "TLSSingleton.h"

OOBase::Thread::Thread() :
	m_hThread(0)
{
}

OOBase::Thread::~Thread()
{
	abort();
}

bool OOBase::Thread::is_running()
{
	Guard<Mutex> guard(m_lock);

	if (!m_hThread.is_valid())
		return false;

	DWORD dwWait = WaitForSingleObject(m_hThread,0);
	if (dwWait == WAIT_TIMEOUT)
	{
		CloseHandle(m_hThread.detach());
		return true;
	}
	else if (dwWait != WAIT_OBJECT_0)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

void OOBase::Thread::run(int (*thread_fn)(void*), void* param)
{
	Guard<Mutex> guard(m_lock);

	assert(!is_running());

	// Close any open handles, this allows restarting
	if (m_hThread.is_valid())
		CloseHandle(m_hThread.detach());

	wrapper wrap;
	wrap.thread_fn = thread_fn;
	wrap.param = param;
	wrap.hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
	if (!wrap.hEvent)
		OOBase_CallCriticalFailure(GetLastError());

	// Start the thread
	m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL,0,&oobase_thread_fn,&wrap,0,NULL));
	if (!m_hThread)
		OOBase_CallCriticalFailure(GetLastError());

	// Wait for the started signal or the thread terminating early
	HANDLE handles[2] =
	{
		wrap.hEvent,
		m_hThread
	};
	DWORD dwWait = WaitForMultipleObjects(2,handles,FALSE,INFINITE);

	if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_OBJECT_0+1)
		OOBase_CallCriticalFailure(GetLastError());
}

bool OOBase::Thread::join(const timeval_t* wait)
{
	Guard<Mutex> guard(m_lock);

	if (!m_hThread.is_valid())
		return true;

	DWORD dwWait = WaitForSingleObject(m_hThread,(wait ? wait->msec() : INFINITE));
	if (dwWait == WAIT_OBJECT_0)
	{
		CloseHandle(m_hThread.detach());
		return true;
	}
	else if (dwWait != WAIT_TIMEOUT)
		OOBase_CallCriticalFailure(GetLastError());

	return false;
}

void OOBase::Thread::abort()
{
	Guard<Mutex> guard(m_lock);

	if (is_running())
		TerminateThread(m_hThread,1);
}

unsigned int OOBase::Thread::oobase_thread_fn(void* param)
{
	wrapper* wrap = static_cast<wrapper*>(param);

	// Copy the values out before we signal
	int (*thread_fn)(void*) = wrap->thread_fn;
	void* p = wrap->param;

	// Set the event, meaning we have started
	if (!SetEvent(wrap->hEvent))
		OOBase_CallCriticalFailure(GetLastError());

	unsigned int ret = static_cast<unsigned int>((*thread_fn)(p));

	// Make sure we clean up any thread-local storage
	OOBase::TLS::ThreadExit();

	return ret;
}

#elif defined(HAVE_PTHREAD)

OOBase::Thread::Thread() :
	m_running(false)
{
	int err = pthread_cond_init(&m_condition,NULL);
	if (err)
		OOBase_CallCriticalFailure(err);
}

OOBase::Thread::~Thread()
{
	abort();

	pthread_cond_destroy(&m_condition);
}

bool OOBase::Thread::is_running()
{
	Guard<Mutex> guard(m_lock);

	return m_running;
}

void OOBase::Thread::run(int (*thread_fn)(void*), void* param)
{
	Guard<Mutex> guard(m_lock);

	assert(!m_running);

	wrapper wrap;
	wrap.pThis = this;
	wrap.thread_fn = thread_fn;
	wrap.param = param;

	pthread_attr_t attr;
	int err = pthread_attr_init(&attr);
	if (err)
		OOBase_CallCriticalFailure(err);

	// Start the thread
	err = pthread_create(&m_thread,NULL,&oobase_thread_fn,&wrap);
	if (err)
		OOBase_CallCriticalFailure(err);

	pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
	err = pthread_mutex_lock(&start_mutex);
	if (err)
		OOBase_CallCriticalFailure(err);

	// Wait for the started signal
	while (!m_running)
	{
		err = pthread_cond_wait(&m_condition,&start_mutex);
		if (err)
			OOBase_CallCriticalFailure(err);
	}

	pthread_mutex_unlock(&start_mutex);
}

bool OOBase::Thread::join(const timeval_t* wait)
{
	Guard<Mutex> guard(m_lock);

	if (wait)
	{
		pthread_mutex_t join_mutex = PTHREAD_MUTEX_INITIALIZER;
		int err = pthread_mutex_lock(&join_mutex);
		if (err)
			OOBase_CallCriticalFailure(err);

		// Wait for the started signal
		while (m_running)
		{
			timespec wt;
			timeval_t now = OOBase::gettimeofday();
			now += *wait;
			wt.tv_sec = now.tv_sec;
			wt.tv_nsec = now.tv_usec * 1000;
			err = pthread_cond_timedwait(&m_condition,&join_mutex,&wt);

			if (err)
			{
				if (err != ETIMEDOUT)
					OOBase_CallCriticalFailure(err);
				break;
			}
		}

		pthread_mutex_unlock(&join_mutex);

		if (err == ETIMEDOUT)
			return false;
	}

	void* ret = 0;
	pthread_join(m_thread,&ret);

	return true;
}

void OOBase::Thread::abort()
{
	Guard<Mutex> guard(m_lock);

	if (m_running)
	{
		int err = pthread_cancel(m_thread);
		if (err)
			OOBase_CallCriticalFailure(err);

		void* ret = 0;
		pthread_join(m_thread,&ret);
	}
}

void* OOBase::Thread::oobase_thread_fn(void* param)
{
	wrapper* wrap = static_cast<wrapper*>(param);

	// Copy the values out before we signal
	Thread* pThis = wrap->pThis;
	int (*thread_fn)(void*) = wrap->thread_fn;
	void* p = wrap->param;

	pThis->m_running = true;

	// Set the event, meaning we have started
	int err = pthread_cond_signal(&pThis->m_condition);
	if (err)
		OOBase_CallCriticalFailure(err);

	int ret = (*thread_fn)(p);

	pThis->m_running = false;

	err = pthread_cond_signal(&pThis->m_condition);
	if (err)
		OOBase_CallCriticalFailure(err);

	return (void*)ret;
}

#else

#error Fix Me!

#endif
