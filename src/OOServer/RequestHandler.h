/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_REQUEST_HANDLER_H_INCLUDED_
#define OOSERVER_REQUEST_HANDLER_H_INCLUDED_

class RequestBase
{
public:
	RequestBase(ACE_HANDLE handle, ACE_InputCDR* input) :
		m_handle(handle),
		m_input(input)
	{}

	virtual ~RequestBase()
	{
		if (m_input)
			delete m_input;
	}

	ACE_HANDLE handle() const
	{
		return m_handle;
	}

	ACE_InputCDR* input() const
	{
		return m_input;
	}

private:
	ACE_HANDLE		m_handle;
	ACE_InputCDR*	m_input;
};

class HandlerBase
{
public:
	virtual ~HandlerBase();

	virtual bool enqueue_root_request(ACE_InputCDR* input, ACE_HANDLE handle) = 0;
	virtual void root_connection_closed(const ACE_CString& key, ACE_HANDLE handle) = 0;
};

// Requests are expected to derive from RequestBase
template <class REQUEST>
class RequestHandler : public HandlerBase
{
protected:
	RequestHandler() :
		m_req_thrd_grp_id(-1),
	    m_thread_count(0),
		m_next_trans_id(1)
	{}

	virtual ~RequestHandler()
	{}

	int start(int threads);
	void stop();

	bool enqueue_request(REQUEST* req);
	bool send_asynch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, const ACE_Message_Block* request, const ACE_Time_Value& deadline);
	bool send_synch(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, const ACE_Message_Block* request, REQUEST*& response, const ACE_Time_Value& deadline);
	bool send_response(ACE_HANDLE handle, ACE_CDR::UShort dest_channel_id, ACE_CDR::ULong trans_id, const ACE_Message_Block* response, const ACE_Time_Value& deadline);
	bool pump_requests(const ACE_Time_Value* deadline = 0);

	virtual void process_request(REQUEST* request, ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, const ACE_Time_Value& request_deadline) = 0;

private:
	RequestHandler(const RequestHandler&) {}
	RequestHandler& operator = (const RequestHandler&) { return *this; }

	struct ThreadParams
	{
		RequestHandler* pThis;
		ACE_Time_Value  deadline;
		ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long> refcount;
	};

	int                                           m_req_thrd_grp_id;
	ACE_Atomic_Op<ACE_Thread_Mutex,long>          m_thread_count;
	ACE_Atomic_Op<ACE_Thread_Mutex,unsigned long> m_next_trans_id;
	ACE_Message_Queue_Ex<REQUEST,ACE_MT_SYNCH>    m_msg_queue;
	ACE_Thread_Mutex                              m_spawn_lock;

	bool wait_for_response(ACE_CDR::ULong trans_id, REQUEST*& response, const ACE_Time_Value* deadline = 0);
	bool build_header(ACE_CDR::UShort dest_channel_id, ACE_CDR::UShort src_channel_id, ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_Message_Block* mb, const ACE_Time_Value& deadline);

	static ACE_THR_FUNC_RETURN request_worker_fn(void* param);
};

#include "./RequestHandler.inl"

#endif  // OOSERVER_REQUEST_HANDLER_H_INCLUDED_

