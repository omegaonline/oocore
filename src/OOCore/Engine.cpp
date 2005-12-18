#include "./Engine.h"

#include <memory>

#include <ace/Get_Opt.h>
#ifdef ACE_WIN32
#include <ace/WFMO_Reactor.h>
#else
#include <ace/TP_Reactor.h>
#endif
#include <ace/OS.h>

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
	int threads = 2;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('e'):
			threads = ACE_OS::atoi(cmd_opts.opt_arg());
			if (threads<0 || threads>10)
			{
				errno = EINVAL;
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Bad number of engine threads '%s' range is [0..10].\n"),cmd_opts.opt_arg()),-1);
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

	return activate(nThreads);
}

int 
OOCore::Engine::close()
{
	if (!m_reactor)
		return -1;

	m_stop = true;

	while (!m_activ_queue.is_empty())
	{
		pump_request_i();
	}

	m_activ_queue.queue()->deactivate();

	if (m_reactor->end_reactor_event_loop() != 0)
		return -1;

	return wait();
}

int 
OOCore::Engine::svc()
{
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
	ACE_Countdown_Time countdown(timeout);
	while ((timeout && *timeout != ACE_Time_Value::zero) || !timeout)
	{
		if (cond_fn && cond_fn(cond_fn_args))
			return 0;
				
		if (pump_request_i(timeout) != 0)
			return -1;
		
		countdown.update();
	}

	errno = EWOULDBLOCK;
	return -1;
}

int 
OOCore::Engine::pump_request_i(ACE_Time_Value* timeout)
{
	ACE_Method_Request* req_;

	if (timeout)
	{
		ACE_Time_Value abs_wait(ACE_OS::gettimeofday());

		abs_wait += *timeout; 
		req_ = m_activ_queue.dequeue(&abs_wait);
	}
	else
		req_ = m_activ_queue.dequeue();

	if (!req_)
	{
		if (errno!=EWOULDBLOCK && errno!=ESHUTDOWN)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to dequeue async request\n")),-1);

		return -1;
	}

	std::auto_ptr<ACE_Method_Request> req(req_);
	return req->call();
}

int 
OOCore::Engine::post_request(ACE_Method_Request* req, ACE_Time_Value* wait)
{
	if (m_stop)
		return -1;

	return m_activ_queue.enqueue(req,wait)==-1 ? -1 : 0;
}
