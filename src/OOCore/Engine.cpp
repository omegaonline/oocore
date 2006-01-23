#include "./Engine.h"

#include <ace/Get_Opt.h>
#ifdef ACE_WIN32
#include <ace/WFMO_Reactor.h>
#else
#include <ace/TP_Reactor.h>
#endif
#include <ace/OS.h>

#include <memory>

OOCore::Engine::Engine(void) :
	m_activ_queue(msg_queue()),
	m_reactor(0),
	m_stop(false)
{
	// Because the object manager uses rand()
	ACE_OS::srand(static_cast<u_int>(ACE_OS::time()));
}

OOCore::Engine::~Engine(void)
{
	if (m_reactor)
		delete m_reactor;
}

const ACE_TCHAR* 
OOCore::Engine::dll_name()
{
	return ACE_TEXT("OOCore");
}

const ACE_TCHAR* 
OOCore::Engine::name()
{
	return ACE_TEXT("Engine");
}

int 
OOCore::Engine::open(int argc, ACE_TCHAR* argv[])
{
	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":e:"));
	int option;
	int threads = ACE_OS::num_processors()+1;
	if (threads==0)
		threads = 2;

	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('e'):
			threads = ACE_OS::atoi(cmd_opts.opt_arg());
			if (threads<1 || threads>10)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of engine threads '%s' range is [1..10].\n"),cmd_opts.opt_arg()),-1);
			}
			break;

		case ACE_TEXT(':'):
			errno = EINVAL;
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	return open(threads);
}

int 
OOCore::Engine::open(unsigned int nThreads)
{
	if (nThreads<0 || nThreads>10)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of engine threads '%u' range is [0..10].\n"),nThreads),-1);
	}

	if (m_reactor)
	{
		errno = EISCONN;
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Engine already open\n")),-1);
	}

	// Create the correct reactor impl
	ACE_Reactor_Impl* reactor_impl;
#ifdef ACE_WIN32
	ACE_NEW_RETURN(reactor_impl,ACE_WFMO_Reactor,-1);
#else
	ACE_NEW_RETURN(reactor_impl,ACE_TP_Reactor,-1);
#endif // !ACE_WIN32

	ACE_NEW_NORETURN(m_reactor,ACE_Reactor(reactor_impl,1));
	if (!m_reactor)
		return -1;

	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED,nThreads);
}

int 
OOCore::Engine::close()
{
	if (!m_reactor)
		return -1;

	m_stop = true;

	while (!m_activ_queue.is_empty())
	{
		ACE_OS::sleep(0);
	}

	m_activ_queue.queue()->deactivate();

	if (m_reactor->end_reactor_event_loop() != 0)
		return -1;

	// Wait a bit, in case something got stuck!
	ACE_OS::sleep(1);

	return wait();
}

int 
OOCore::Engine::svc()
{
	m_reactor->owner(ACE_Thread::self());

	return m_reactor->run_reactor_event_loop();
}

ACE_Reactor* 
OOCore::Engine::reactor()
{
	return m_reactor;
}

int 
OOCore::Engine::pump_requests(ACE_Time_Value* timeout, CONDITION_FN cond_fn, void* cond_fn_args)
{
	cond_req* c = 0;
	if (cond_fn)
	{
		if (!m_nestcount->first)
		{
			m_nestcount->first = true;
			m_nestcount->second = 0;
		}

		ACE_NEW_RETURN(c,cond_req(cond_fn,cond_fn_args,++m_nestcount->second),-1);
		
		m_conditions.push_back(c);
	}	

	ACE_Countdown_Time countdown(timeout);
	while ((timeout && *timeout != ACE_Time_Value::zero) || !timeout)
	{
		int ret = check_conditions();
		if (ret==0)
		{
			if (timeout && timeout->sec()>1)
			{
				ACE_Time_Value wait(1);
				ret = pump_request_i(&wait);
				if (ret==-1 && errno==EWOULDBLOCK)
					ret = 0;
			}
			else
				ret = pump_request_i(timeout);
		}

		if (ret == 1)
			return 0;
		else if (ret != 0)
			goto exit;

		countdown.update();
	}

	errno = EWOULDBLOCK;
	
exit:
	if (c)
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

		for (std::list<cond_req*>::iterator i=m_conditions.begin();i!=m_conditions.end();++i)
		{
			if (*i == c)
			{
				delete (*i);
				m_conditions.erase(i);
				break;
			}
		}
	}

	return -1;
}

int
OOCore::Engine::check_conditions()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock,0);

	if (!guard.locked())
	{
		ACE_OS::sleep(0);
		return 0;
	}

	for (std::list<cond_req*>::iterator i=m_conditions.begin();i!=m_conditions.end();++i)
	{
		if ((*i)->cond_fn((*i)->cond_fn_args))
		{
			cond_req* r = *i;
			m_conditions.erase(i);

			return r->call();
		}
	}

	return 0;
}

int 
OOCore::Engine::cond_req::call()
{
	if (ACE_Thread::self()==tid && ENGINE::instance()->m_nestcount->second==nesting)
	{
		--ENGINE::instance()->m_nestcount->second;
		delete this;
		return 1;
	}
	
	if (ENGINE::instance()->post_request(this) != 0)
		delete this;

	return 0;
}

int 
OOCore::Engine::pump_request_i(ACE_Time_Value* timeout)
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
		if (errno!=EWOULDBLOCK && errno!=ESHUTDOWN)
			ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Failed to dequeue async request\n")));

		return -1;
	}

	int ret = req->call();
	if (ret != 0 && ret != 1)
		ACE_ERROR((LM_DEBUG,ACE_TEXT("(%P|%t) Async request call() returned %d\n"),ret));

	return ret;
}

int 
OOCore::Engine::post_request(ACE_Method_Request* req, ACE_Time_Value* wait)
{
	if (m_stop)
		return -1;

	return m_activ_queue.enqueue(req,wait)==-1 ? -1 : 0;
}
