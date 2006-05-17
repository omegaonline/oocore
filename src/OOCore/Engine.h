#ifndef OOCORE_ENGINE_H_INCLUDED_
#define OOCORE_ENGINE_H_INCLUDED_

#include <ace/Task.h>
#include <ace/Reactor.h>
#include <ace/Singleton.h>
#include <ace/Activation_Queue.h>
#include <ace/Method_Request.h>
#include <ace/Thread.h>
#include <ace/TSS_T.h>

#include <list>

#include "./OOCore.h"

namespace OOCore
{
namespace Impl
{

class Engine : 
	public ACE_Task<ACE_MT_SYNCH>
{
public:
	int open(int argc, ACE_TCHAR* argv[]);
	int open(unsigned int nThreads);
	int close();

	ACE_Reactor* reactor();

	int pump_requests(ACE_Time_Value* timeout = 0, PUMP_CONDITION_FN cond_fn = 0, void* cond_fn_args = 0);
	int post_request(ACE_Method_Request* req, ACE_Time_Value* wait = 0);

private:
	Engine();
	virtual ~Engine();
	friend class ACE_DLL_Singleton<Engine, ACE_Thread_Mutex>;

	struct cond_req : ACE_Method_Request
	{
		cond_req() :
			tid(ACE_Thread::self()), nesting(0)
		{}

		cond_req(PUMP_CONDITION_FN fn, void* args, long n) :
			cond_fn(fn), cond_fn_args(args), tid(ACE_Thread::self()), nesting(n)
		{}

		cond_req& operator = (const cond_req& rhs)
		{
			cond_fn = rhs.cond_fn;
			cond_fn_args = rhs.cond_fn_args;
			tid = rhs.tid;
			nesting = rhs.nesting;

			return *this;
		}
			
		PUMP_CONDITION_FN cond_fn;
		void* cond_fn_args;
		ACE_thread_t tid;
		long nesting;

		int call();
	};
	friend struct cond_req;

	ACE_Activation_Queue m_activ_queue;
	ACE_Reactor* m_reactor;
	bool m_stop;
	ACE_TSS<std::pair<bool,long> > m_nestcount;
	ACE_Thread_Mutex m_lock;
	std::list<cond_req*> m_conditions;
		
	int svc();
	int pump_request_i(ACE_Time_Value* timeout);
	int check_conditions();
};

typedef ACE_DLL_Singleton<Engine, ACE_Thread_Mutex> ENGINE;

};
};

#endif // OOCORE_ENGINE_H_INCLUDED_
