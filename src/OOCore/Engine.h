#ifndef _OOCORE_ENGINE_H_INCLUDED_
#define _OOCORE_ENGINE_H_INCLUDED_

#include <ace/Task.h>
#include <ace/Reactor.h>
#include <ace/Singleton.h>
#include <ace/Activation_Queue.h>

#include "./OOCore_export.h"

class OOCore_Export OOCore_Engine : public ACE_Task<ACE_MT_SYNCH>
{
public:
	OOCore_Engine(void);
	virtual ~OOCore_Engine(void);

	int open(int argc, ACE_TCHAR* argv[]);
	int open();
	int shutdown();

	const ACE_TCHAR* dll_name(void);
	const ACE_TCHAR* name(void);

	ACE_Reactor* reactor();

	typedef bool (*CONDITION_FN)(void* cond_fn_args);
	int pump_requests(ACE_Time_Value* timeout = 0, CONDITION_FN cond_fn = 0, void* cond_fn_args = 0);
	int post_request(ACE_Method_Request* req, ACE_Time_Value* wait = 0);

private:
	ACE_Activation_Queue m_activ_queue;
	ACE_Reactor* m_reactor;
	bool m_stop;
	
	int svc();
	int pump_request_i(ACE_Time_Value* timeout = 0);
};

typedef ACE_DLL_Singleton_T<OOCore_Engine, ACE_Thread_Mutex> ENGINE;
OOCORE_SINGLETON_DECLARE(ACE_DLL_Singleton_T,OOCore_Engine,ACE_Thread_Mutex);

#endif // _OOCORE_ENGINE_H_INCLUDED_
