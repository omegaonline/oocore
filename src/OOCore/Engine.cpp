#include "OOCore_precomp.h"

#include "./Engine.h"
#include "./StdApartment.h"

using namespace Omega;
using namespace OTL;

// Engine
Engine::Engine(void) :
	m_activ_queue(msg_queue()),
	m_reactor(0),
	m_stop(false),
	m_reactor_start_count(0),
	m_pump_start_count(0)
{
}

Engine::~Engine(void)
{
	if (m_reactor)
		delete m_reactor;
}

Engine* Engine::GetSingleton()
{
	return ENGINE::instance();
}

int Engine::open(void *args)
{
	ACE_UNUSED_ARG(args);

	if (!m_stop)
		++m_pump_start_count;

	return 0;
}

void Engine::close()
{
	if (--m_pump_start_count==0)
	{
		m_stop = true;

		while (!m_activ_queue.is_empty())
		{
			ACE_OS::sleep(1);
		}

		m_activ_queue.queue()->deactivate();
	}
}

ACE_Reactor* Engine::get_reactor()
{
	if (++m_reactor_start_count==1)
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

		// Create the correct reactor impl
		ACE_Reactor_Impl* reactor_impl;
	#ifdef OMEGA_WIN32
		OMEGA_NEW(reactor_impl,ACE_WFMO_Reactor);
	#else
		OMEGA_NEW(reactor_impl,ACE_TP_Reactor);
	#endif // !OMEGA_WIN32

		ACE_Auto_Ptr<ACE_Reactor_Impl> ptrReactor(reactor_impl);
		OMEGA_NEW(m_reactor,ACE_Reactor(reactor_impl,1));
		ptrReactor.release();

		unsigned int threads = static_cast<unsigned int>(ACE_OS::num_processors()+1);
		if (threads < 2)
			threads = 2;

		if (activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED,threads) != 0)
			OOCORE_THROW_LASTERROR();
	}

	return m_reactor;
}

void Engine::release_reactor()
{
	if (--m_reactor_start_count==0 && m_reactor)
	{
		m_reactor->end_event_loop();

		thr_mgr()->wait_grp(grp_id());
	}
}

const ACE_TCHAR* Engine::name()
{
	return ACE_TEXT("Engine");
}

const ACE_TCHAR* Engine::dll_name()
{
	return ACE_TEXT("OOCore");
}

int Engine::ConditionRequest::call()
{
	try
	{
		if (ACE_Thread::self()==tid && ENGINE::instance()->m_nestcount->second==nesting)
		{
			--ENGINE::instance()->m_nestcount->second;
			delete this;
			return 1;
		}
		
		if (!ENGINE::instance()->post_request(this))
		{
			delete this;
			return 1;
		}

		return 0;
	}
	catch (IException* pE)
	{
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("ConditionRequest call() throws an exception: %s"),(const char*)pE->Description()));
		pE->Release();
	}
	catch (...)
	{}

	delete this;
	return 1;	
}

// false = timedout
bool Engine::pump_requests(ACE_Time_Value* timeout, Activation::IApartment::PUMP_CONDITION_FN cond_fn, void* cond_fn_args)
{
	ConditionRequest* c = 0;
	if (cond_fn)
	{
		if (!m_nestcount->first)
		{
			m_nestcount->first = true;
			m_nestcount->second = 0;
		}

		OMEGA_NEW(c,ConditionRequest(cond_fn,cond_fn_args,++m_nestcount->second));

		ACE_Auto_Ptr<ConditionRequest> pc(c);

		m_conditions.push_back(c);

		pc.release();
	}	

	try
	{
		ACE_Countdown_Time countdown(timeout);
		do
		{
			bool ret = check_conditions();
			if (!ret)
			{
				if ((timeout && timeout->sec()>1) || cond_fn)
				{
					ACE_Time_Value wait(1);
					ret = pump_request_i(&wait);
				}
				else
					ret = pump_request_i(timeout);
			}

			if (ret)
				return true;
			
			countdown.update();
		} while ((timeout && *timeout != ACE_Time_Value::zero) || !timeout);
	}
	catch (...)
	{
		if (c)
			remove_condition(c);

		throw;
	}

	if (c)
		remove_condition(c);
	
	return false;
}

void Engine::remove_condition(ConditionRequest* c)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	for (std::list<ConditionRequest*>::iterator i=m_conditions.begin();i!=m_conditions.end();++i)
	{
		if (*i == c)
		{
			delete c;
			m_conditions.erase(i);
			break;
		}
	}
}

int Engine::svc()
{
	return m_reactor->run_reactor_event_loop();
}

bool Engine::check_conditions()
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock,0);

	if (!guard.locked())
	{
		ACE_OS::sleep(0);
		return false;
	}

	for (std::list<ConditionRequest*>::iterator i=m_conditions.begin();i!=m_conditions.end();++i)
	{
		if ((*i)->cond_fn((*i)->cond_fn_args))
		{
			ConditionRequest* r = *i;
			m_conditions.erase(i);

			return (r->call() == 1);
		}
	}

	return false;
}

bool Engine::pump_request_i(ACE_Time_Value* timeout)
{
	ACE_Method_Request* req;

	if (timeout)
	{
		ACE_Time_Value abs_wait(ACE_OS::gettimeofday());

		abs_wait += *timeout; 
		req = m_activ_queue.dequeue(&abs_wait);
	}
	else
		req = m_activ_queue.dequeue();

	if (!req)
	{
		int last_error = ACE_OS::last_error();
		if (last_error!=EWOULDBLOCK && last_error!=ESHUTDOWN)
			OOCORE_THROW_LASTERROR();
		
		if (last_error == ESHUTDOWN)
			return true;
		
		return false;
	}

	return (req->call() == 1);
}

bool Engine::post_request(ACE_Method_Request* req, ACE_Time_Value* wait)
{
	if (m_stop)
		return false;

	if (m_activ_queue.enqueue(req,wait)==-1)
		OOCORE_THROW_LASTERROR();

	return true;
}
