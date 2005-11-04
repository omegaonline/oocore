#ifndef _OOCORE_CHANNEL_H_INCLUDED_
#define _OOCORE_CHANNEL_H_INCLUDED_

#include <list>

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Method_Request.h>
#include <ace/Message_Queue.h>

#include "./OOCore_export.h"

class OOCore_Channel_Handler;

class OOCore_Export OOCore_Channel
{
	friend class OOCore_Channel_Handler;	// for bind_handler()
	friend class OOCore_Transport_Base;

public:
	int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0);
	int close();
	
private:
	OOCore_Channel();
	virtual ~OOCore_Channel();

	struct msg_param : ACE_Method_Request
	{
		OOCore_Channel* pt;
		ACE_Message_Block* mb;

		int call()
		{
			return pt->recv_i(mb);
		}
	};
	friend struct msg_param;

	ACE_Thread_Mutex m_lock;
	OOCore_Channel* m_sibling;
	OOCore_Channel_Handler* m_handler;
	ACE_Message_Queue<ACE_MT_SYNCH>	m_msg_queue;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;

	enum close_flags
	{
		NOT_CLOSED = 0,
		SEND_CLOSED = 1,
		RECV_CLOSED = 2,
		READY_TO_CLOSE = (SEND_CLOSED | RECV_CLOSED),
		CLOSED = 4
	};
	unsigned int m_close_flags;
			
	int bind_handler(OOCore_Channel_Handler* handler);
	int post_msg(ACE_Message_Block* mb, ACE_Time_Value* wait);
	int recv_i(ACE_Message_Block* mb);
	int close_i(bool bRecv);
	
	static int create(OOCore_Channel*& acceptor_channel, OOCore_Channel*& handler_channel);
};

#endif // _OOCORE_CHANNEL_H_INCLUDED_
