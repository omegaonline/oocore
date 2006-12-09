#include "OOCore_precomp.h"

#include "./StdApartment.h"
#include "./Engine.h"

using namespace Omega;
using namespace OTL;

StdApartment::StdApartment()
{
	Engine::GetSingleton()->open();
}

StdApartment::~StdApartment()
{
	Engine::GetSingleton()->close();
}

bool_t StdApartment::PumpRequests(uint32_t* timeout, PUMP_CONDITION_FN cond_fn, void* cond_fn_args)
{
	ACE_Time_Value* pTimeout = 0;
	if (timeout)
	{
		ACE_Time_Value time_val(*timeout);
		pTimeout = &time_val;
	}

	return Engine::GetSingleton()->pump_requests(pTimeout,cond_fn,cond_fn_args);
}

bool_t StdApartment::PostRequest(REQUEST_FN request_fn, void* request_fn_args, uint32_t wait)
{
	Request* req;
	OMEGA_NEW(req,Request(request_fn,request_fn_args));

	ACE_Time_Value* pTimeout = 0;
	if (wait)
	{
		ACE_Time_Value time_val(wait);
		pTimeout = &time_val;
	}

	return Engine::GetSingleton()->post_request(req,pTimeout);
}
