#pragma once

#include <ace/Condition_Thread_Mutex.h>
#include <ace/Message_Queue.h>

#include "./OOCore_export.h"

class OOCore_Channel_Handler;

// TODO This should not be using ACE_Message_Queue notification
// it should be using OOCore_PostRequest

class OOCore_Export OOCore_Channel
{
	friend class OOCore_Channel_Handler;	// for bind_handler()
	friend class OOCore_Transport_Base;

public:
	virtual int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0);
	virtual int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0);
	virtual int close(bool immediate);

private:
	OOCore_Channel();
	virtual ~OOCore_Channel();

	ACE_Message_Queue<ACE_MT_SYNCH> m_msg_queue;
	OOCore_Channel* m_sibling;
	OOCore_Channel_Handler* m_handler;

	int bind_handler(OOCore_Channel_Handler* handler);
	
	static int create(OOCore_Channel*& acceptor_channel, OOCore_Channel*& handler_channel);
};
