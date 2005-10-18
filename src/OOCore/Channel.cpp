#include "./Channel.h"

#include <ace/Reactor_Notification_Strategy.h>
#include <ace/Reactor.h>

#include "./Channel_Handler.h"

OOCore_Channel::OOCore_Channel() :
	m_sibling(0),
	m_handler(0)
{
}

OOCore_Channel::~OOCore_Channel()
{
}

int OOCore_Channel::create(OOCore_Channel*& acceptor_channel, OOCore_Channel*& handler_channel)
{
	ACE_NEW_RETURN(acceptor_channel,OOCore_Channel,-1);
	ACE_NEW_NORETURN(handler_channel,OOCore_Channel);
	if (handler_channel == 0)
	{
		delete acceptor_channel;
		return -1;
	}

	acceptor_channel->m_sibling = handler_channel;
	handler_channel->m_sibling = acceptor_channel;

	return 0;
}

int OOCore_Channel::bind_handler(OOCore_Channel_Handler* handler)
{
	if (m_handler != 0)
		return -1;

    // Create a new strategy
	ACE_Reactor_Notification_Strategy* strategy;
	ACE_NEW_RETURN(strategy,ACE_Reactor_Notification_Strategy(ACE_Reactor::instance(),handler,ACE_Event_Handler::READ_MASK),-1);
	m_msg_queue.notification_strategy(strategy);
	
	m_handler = handler;
	m_handler->addref();

	return 0;
}

int OOCore_Channel::recv(ACE_Message_Block*& mb, ACE_Time_Value* wait)
{
	if (m_msg_queue.dequeue_head(mb,wait) == -1)
		return -1;

	if (mb->msg_type()==ACE_Message_Block::MB_HANGUP)
	{
		mb->release();
		m_sibling = 0;
		return -1;
	}
	
	return 0;
}

int OOCore_Channel::send(ACE_Message_Block* mb, ACE_Time_Value* wait)
{
	return (m_sibling->m_msg_queue.enqueue_prio(mb,wait) == -1 ? -1 : 0);
}

int OOCore_Channel::close(bool immediate)
{
	if (m_sibling != 0)
	{
		if (!immediate)
		{
			ACE_Message_Block* mb;
			ACE_NEW_RETURN(mb,ACE_Message_Block(0,ACE_Message_Block::MB_HANGUP),-1);
			
			if (m_sibling->m_msg_queue.enqueue_tail(mb) == -1)
			{
				mb->release();
				return -1;
			}
		}
		else
		{
			m_sibling->m_sibling = 0;
			m_sibling->close(true);
		}
	}

	if (m_handler != 0)
	{
		delete m_msg_queue.notification_strategy();
		m_msg_queue.notification_strategy(0);
		m_handler->handle_close();
		m_handler->release();
	}
	delete this;

	return 0;
}