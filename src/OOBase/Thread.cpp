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

#endif

namespace
{

#if defined(_WIN32)

	class Win32Thread : public OOBase::detail::ThreadImpl
	{
	public:
		Win32Thread();
		virtual ~Win32Thread();

		virtual void run(int (*thread_fn)(void*), void* param);
		virtual bool join(const OOBase::timeval_t* wait = 0);
		virtual void abort();
		virtual bool is_running();

	private:
		struct wrapper
		{
			OOBase::Win32::SmartHandle m_hEvent;
			int                      (*m_thread_fn)(void*);
			void*                      m_param;
		};

		OOBase::Mutex              m_lock;
		OOBase::Win32::SmartHandle m_hThread;

		static unsigned int __stdcall oobase_thread_fn(void* param);
	};

	Win32Thread::Win32Thread() :
		m_hThread(0)
	{
	}

	Win32Thread::~Win32Thread()
	{
		abort();
	}

	bool Win32Thread::is_running()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

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

	void Win32Thread::run(int (*thread_fn)(void*), void* param)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		assert(!is_running());

		// Close any open handles, this allows restarting
		if (m_hThread.is_valid())
			CloseHandle(m_hThread.detach());

		wrapper wrap;
		wrap.m_thread_fn = thread_fn;
		wrap.m_param = param;
		wrap.m_hEvent = CreateEventW(NULL,TRUE,FALSE,NULL);
		if (!wrap.m_hEvent)
			OOBase_CallCriticalFailure(GetLastError());

		// Start the thread
		m_hThread = reinterpret_cast<HANDLE>(_beginthreadex(NULL,0,&oobase_thread_fn,&wrap,0,NULL));
		if (!m_hThread)
			OOBase_CallCriticalFailure(GetLastError());

		// Wait for the started signal or the thread terminating early
		HANDLE handles[2] =
		{
			wrap.m_hEvent,
			m_hThread
		};
		DWORD dwWait = WaitForMultipleObjects(2,handles,FALSE,INFINITE);

		if (dwWait != WAIT_OBJECT_0 && dwWait != WAIT_OBJECT_0+1)
			OOBase_CallCriticalFailure(GetLastError());
	}

	bool Win32Thread::join(const OOBase::timeval_t* wait)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

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

	void Win32Thread::abort()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		if (is_running())
			TerminateThread(m_hThread,1);
	}

	unsigned int Win32Thread::oobase_thread_fn(void* param)
	{
		wrapper* wrap = static_cast<wrapper*>(param);

		// Copy the values out before we signal
		int (*thread_fn)(void*) = wrap->m_thread_fn;
		void* p = wrap->m_param;

		// Set the event, meaning we have started
		if (!SetEvent(wrap->m_hEvent))
			OOBase_CallCriticalFailure(GetLastError());

		unsigned int ret = static_cast<unsigned int>((*thread_fn)(p));

		// Make sure we clean up any thread-local storage
		OOBase::TLS::ThreadExit();

		return ret;
	}

#endif // _WIN32

#if defined(HAVE_PTHREAD)

	class PthreadThread : public OOBase::detail::ThreadImpl
	{
	public:
		PthreadThread();
		virtual ~PthreadThread();

		virtual void run(int (*thread_fn)(void*), void* param);
		virtual bool join(const OOBase::timeval_t* wait = 0);
		virtual void abort();
		virtual bool is_running();

	private:
		struct wrapper
		{
			Thread*            m_pThis;
			int                (*m_thread_fn)(void*);
			void*              m_param;
		};

		OOBase::Mutex  m_lock;
		bool           m_running;
		pthread_t      m_thread;
		pthread_cond_t m_condition;

		static void* oobase_thread_fn(void* param);
	};

	PthreadThread::PthreadThread() :
		m_running(false)
	{
		int err = pthread_cond_init(&m_condition,NULL);
		if (err)
			OOBase_CallCriticalFailure(err);
	}

	PthreadThread::~PthreadThread()
	{
		abort();

		pthread_cond_destroy(&m_condition);
	}

	bool PthreadThread::is_running()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		return m_running;
	}

	void PthreadThread::run(int (*thread_fn)(void*), void* param)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		assert(!m_running);

		wrapper wrap;
		wrap.m_pThis = this;
		wrap.m_thread_fn = thread_fn;
		wrap.m_param = param;

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

	bool PthreadThread::join(const OOBase::timeval_t* wait)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

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

		if (m_running)
		{
			void* ret = 0;
			pthread_join(m_thread,&ret);
		}

		return true;
	}

	void PthreadThread::abort()
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		if (m_running)
		{
			int err = pthread_cancel(m_thread);
			if (err)
				OOBase_CallCriticalFailure(err);

			void* ret = 0;
			pthread_join(m_thread,&ret);
		}
	}

	void* PthreadThread::oobase_thread_fn(void* param)
	{
		wrapper* wrap = static_cast<wrapper*>(param);

		// Copy the values out before we signal
		Thread* pThis = wrap->m_pThis;
		int (*thread_fn)(void*) = wrap->m_thread_fn;
		void* p = wrap->m_param;

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

#endif // HAVE_PTHREAD

}

OOBase::Thread::Thread() : 
	m_impl(0)
{
#if defined(_WIN32)
	OOBASE_NEW(m_impl,Win32Thread());
#elif (defined(HAVE_PTHREAD)
	OOBASE_NEW(m_impl,PthreadThread());
#else
#error Fix me!
#endif

	if (!m_impl)
		OOBase_OutOfMemory();
}

OOBase::Thread::~Thread()
{
	delete m_impl;
}

void OOBase::Thread::run(int (*thread_fn)(void*), void* param)
{
	return m_impl->run(thread_fn,param);
}

bool OOBase::Thread::join(const timeval_t* wait)
{
	return m_impl->join(wait);
}

void OOBase::Thread::abort()
{
	return m_impl->abort();
}

bool OOBase::Thread::is_running()
{
	return m_impl->is_running();
}
