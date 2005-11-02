#include "./OOCore.h"

#include <ace/CDR_Stream.h>
#include <ace/Active_Map_Manager.h>
#include <ace/Reactor.h>
#include <ace/Countdown_Time.h>

ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_Active_Map_Manager_Key& key)
{
	// Read the channel key
	key.decode(input.rd_ptr());
	if (!input.skip_bytes(ACE_Active_Map_Manager_Key::size()))
		return false;

	return true;
}

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_Active_Map_Manager_Key& key)
{
	ACE_CDR::Octet* buf;
	ACE_NEW_RETURN(buf,ACE_CDR::Octet[ACE_Active_Map_Manager_Key::size()],false);
	
	key.encode(buf);

	bool ret = output.write_octet_array(buf,ACE_Active_Map_Manager_Key::size());
	delete [] buf;
	if (!ret)
		return false;
		
	return true;	
}

ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_CDR::Boolean& val)
{
	return input.read_boolean(val);
}

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_CDR::Boolean& val)
{
	return output.write_boolean(val);
}

#include "./Transport_Service.h"
#include "./Client_Service.h"

extern "C" int OOCore_Export CreateStub(const OOObj::GUID& iid, OOObj::Object* obj, OOCore_Object_Stub_Base** stub)
{
	if (iid==OOCore_Transport_Service::IID)
		ACE_NEW_RETURN(*stub,OOCore_Transport_Service_Stub(obj),-1);
	else if (iid==OOObj::Client_Service::IID)
		ACE_NEW_RETURN(*stub,OOObj::Client_Service_Stub(obj),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Stub IID\n")),-1);
	
	return 0;
}

extern "C" int OOCore_Export CreateProxy(const OOObj::GUID& iid, const OOObj::cookie_t& key, OOCore_ProxyStub_Handler* handler, OOObj::Object** proxy)
{
	if (iid==OOCore_Transport_Service::IID)
		ACE_NEW_RETURN(*proxy,OOCore_Transport_Service_Proxy(key,handler),-1);
	else if (iid==OOObj::Client_Service::IID)
		ACE_NEW_RETURN(*proxy,OOObj::Client_Service_Proxy(key,handler),-1);
	else
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid Proxy IID\n")),-1);
	
	(*proxy)->AddRef();
	return 0;
}

OOCore_Export void* OOCore_Alloc(size_t size)
{
	return ACE_OS::malloc(size);
}

OOCore_Export void OOCore_Free(void* p)
{
	ACE_OS::free(p);
}

static ACE_Activation_Queue activ_queue;

int OOCore_Export OOCore_PostRequest(ACE_Method_Request* req, ACE_Time_Value* wait)
{
	int ret = activ_queue.enqueue(req,wait)==-1 ? -1 : 0;
	if (ret==0)
	{
		// Wake up one thread
		ACE_Reactor::instance()->notify();
	}

	return ret;
}

static int OOCore_Reactor_Event_Hook(ACE_Reactor * reactor)
{
	while (!activ_queue.is_empty())
	{
		ACE_Method_Request* req_ = activ_queue.dequeue();
		if (!req_ && errno!=EWOULDBLOCK)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to dequeue async request\n")),-1);

		std::auto_ptr<ACE_Method_Request> req(req_);
		if (req->call() != 0)
			return -1;
	}
	return 0;
}

int OOCore_Export OOCore_RunReactor(ACE_Time_Value* timeout)
{
	return OOCore_RunReactorEx(timeout);
}

int OOCore_Export OOCore_RunReactorEx(ACE_Time_Value* timeout, CONDITION_FN cond_fn, void* p)
{
	if (!cond_fn)
	{
		if (timeout)
			return ACE_Reactor::instance()->run_reactor_event_loop(*timeout,&OOCore_Reactor_Event_Hook);
		else
			return ACE_Reactor::instance()->run_reactor_event_loop(&OOCore_Reactor_Event_Hook);
	}
	else
	{
		if (!timeout)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) RunReactorEx with condition requires a timeout\n")),-1);

		if (ACE_Reactor::instance()->reactor_event_loop_done ())
			return 0;

		ACE_Countdown_Time countdown(timeout);
		while (*timeout != ACE_Time_Value::zero)
		{
			if (cond_fn(p))
				return 0;
					
			int result = ACE_Reactor::instance()->handle_events(*timeout);
			if (OOCore_Reactor_Event_Hook(ACE_Reactor::instance()))
				continue;
			else if (result == -1)
				return -1;

			countdown.update();
		}

		errno = EWOULDBLOCK;
		return -1;
	}
}

static ACE_Atomic_Op<ACE_Thread_Mutex,long> depthcount(0);
static ACE_Activation_Queue close_queue;

void OOCore_IncCallDepth()
{
	++depthcount;
}

int OOCore_DecCallDepth()
{
	if (--depthcount==0)
	{
		while (!close_queue.is_empty())
		{
			ACE_Method_Request* req_ = close_queue.dequeue();
			if (!req_ && errno!=EWOULDBLOCK)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to dequeue close request\n")),-1);

			std::auto_ptr<ACE_Method_Request> req(req_);
			if (req->call() != 0)
				return -1;
		}
	}

	return 0;
}

int OOCore_Export OOCore_PostCloseRequest(ACE_Method_Request* req, ACE_Time_Value* wait)
{
	if (depthcount==0)
	{
		/*int ret = req->call();
		delete req;
		return ret;*/
		return OOCore_PostRequest(req,wait);
	}
	else
	{
		return close_queue.enqueue(req,wait)==-1 ? -1 : 0;
	}
}
