/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_REQUEST_HANDLER_H_INCLUDED_
#define OOSERVER_REQUEST_HANDLER_H_INCLUDED_

#include <ace/CDR_Stream.h>
#include <ace/Message_Queue_T.h>
#include <ace/Condition_Thread_Mutex.h>

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

// Requests are expected to derive from RequestBase
template <class REQUEST>
class RequestHandler
{
protected:
	int enqueue_request(REQUEST* req);
	int send_asynch(ACE_HANDLE handle, const ACE_OutputCDR& request, ACE_Time_Value* wait);
	int send_synch(ACE_HANDLE handle, const ACE_OutputCDR& request, REQUEST*& response, ACE_Time_Value* wait);
	int pump_requests(ACE_Time_Value* deadline = 0);
	void stop();

	virtual void process_request(REQUEST* request, ACE_CDR::ULong trans_id, ACE_Time_Value* request_deadline) = 0;
		
private:
	static const unsigned int header_padding = 16;

	int wait_for_response(ACE_HANDLE handle, ACE_CDR::ULong trans_id, REQUEST*& response, ACE_Time_Value* deadline = 0);
	int build_header(ACE_CDR::ULong trans_id, ACE_OutputCDR& header, const ACE_OutputCDR& request, const ACE_Time_Value& deadline);
	ACE_CDR::ULong next_trans_id();
	bool expired_request(ACE_CDR::ULong trans_id);
	void cancel_trans_id(ACE_CDR::ULong trans_id);

	ACE_Message_Queue_Ex<REQUEST,ACE_MT_SYNCH>	m_msg_queue;
};

#include "./RequestHandler.inl"

#endif  // OOSERVER_REQUEST_HANDLER_H_INCLUDED_
