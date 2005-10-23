#pragma once

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
	virtual int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0);
	virtual int close();

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
	bool m_closed;

	int bind_handler(OOCore_Channel_Handler* handler);
	int post_msg(ACE_Message_Block* mb, ACE_Time_Value* wait);
	int recv_i(ACE_Message_Block* mb);
	void release();
	
	static int create(OOCore_Channel*& acceptor_channel, OOCore_Channel*& handler_channel);
};
