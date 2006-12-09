#ifndef OOCORE_ENGINE_H_INCLUDED_
#define OOCORE_ENGINE_H_INCLUDED_

// This is cheeky I know, but the interface is private 
// and subject to change, and therefore not part of the standard API
#include "../OOServer/InterProcess.h"

class Engine : public ACE_Task<ACE_MT_SYNCH>
{
public:
	const ACE_TCHAR* name();
	const ACE_TCHAR* dll_name();

	int open (void *args = 0);
	void close();

	bool pump_requests(ACE_Time_Value* timeout = 0, Omega::Activation::IApartment::PUMP_CONDITION_FN cond_fn = 0, void* cond_fn_args = 0);
	bool post_request(ACE_Method_Request* req, ACE_Time_Value* wait = 0);

	ACE_Reactor* get_reactor();
	void release_reactor();

	static Engine* GetSingleton();

private:
	Engine();
	virtual ~Engine();
	friend class ACE_DLL_Singleton_T<Engine, ACE_Recursive_Thread_Mutex>;

	typedef ACE_DLL_Singleton_T<Engine, ACE_Recursive_Thread_Mutex> ENGINE;

	struct ConditionRequest : ACE_Method_Request
	{
		ConditionRequest() :
			tid(ACE_Thread::self()), nesting(0)
		{}

		ConditionRequest(Omega::Activation::IApartment::PUMP_CONDITION_FN fn, void* args, long n) :
			cond_fn(fn), cond_fn_args(args), tid(ACE_Thread::self()), nesting(n)
		{}

		ConditionRequest& operator = (const ConditionRequest& rhs)
		{
			cond_fn = rhs.cond_fn;
			cond_fn_args = rhs.cond_fn_args;
			tid = rhs.tid;
			nesting = rhs.nesting;

			return *this;
		}
			
		Omega::Activation::IApartment::PUMP_CONDITION_FN cond_fn;
		void* cond_fn_args;
		ACE_thread_t tid;
		long nesting;

		int call();
	};

	ACE_Activation_Queue m_activ_queue;
	ACE_Reactor* m_reactor;
	bool m_stop;
	Omega::AtomicOp<long>::type m_reactor_start_count;
	Omega::AtomicOp<long>::type m_pump_start_count;
	ACE_TSS<std::pair<bool,long> > m_nestcount;
	ACE_Recursive_Thread_Mutex m_lock;
	std::list<ConditionRequest*> m_conditions;
			
	int svc();
	bool pump_request_i(ACE_Time_Value* timeout);
	bool check_conditions();
	void remove_condition(ConditionRequest* c);
};

#endif // OOCORE_ENGINE_H_INCLUDED_