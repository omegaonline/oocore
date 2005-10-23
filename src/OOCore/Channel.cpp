#include "./Channel.h"

#include <ace/Reactor_Notification_Strategy.h>
#include <ace/Reactor.h>

#include "./Channel_Handler.h"
#include "./OOCore.h"

OOCore_Channel::OOCore_Channel() :
	m_sibling(0),
	m_handler(0),
	m_refcount(1),
	m_closed(false)
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
		acceptor_channel->release();
		return -1;
	}

	acceptor_channel->m_sibling = handler_channel;
	handler_channel->m_sibling = acceptor_channel;

	return 0;
}

int OOCore_Channel::bind_handler(OOCore_Channel_Handler* handler)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_handler != 0)
		return -1;

	m_handler = handler;
	m_handler->addref();

	while (!m_msg_queue.is_empty())
	{
		ACE_Message_Block* mb = 0;
		if (m_msg_queue.dequeue_head(mb) != -1)
		{
			if (post_msg(mb,0) != 0)
				mb->release();
		}
	}

	return 0;
}

int OOCore_Channel::send(ACE_Message_Block* mb, ACE_Time_Value* wait)
{
	if (!m_sibling)
		return -1;

	return m_sibling->post_msg(mb,wait);
}

void OOCore_Channel::release()
{
	if (--m_refcount==0)
		delete this;
}

int OOCore_Channel::post_msg(ACE_Message_Block* mb, ACE_Time_Value* wait)
{
	if (m_handler)
	{
		msg_param* p;
		ACE_NEW_RETURN(p,msg_param,-1);

		p->mb = mb;
		p->pt = this;

		++m_refcount;

		if (OOCore_PostRequest(p,wait)!=0)
		{
			release();
			delete p;
			return -1;
		}

		return 0;
	}
	else
	{
		return (m_msg_queue.enqueue_head(mb,wait)==-1 ? -1 : 0);
	}
}

int OOCore_Channel::recv_i(ACE_Message_Block* mb)
{
	int res = -1;

	if (m_handler)
	{
		if (mb->msg_type() == ACE_Message_Block::MB_HANGUP)
		{
			m_sibling = 0;
			res = close();
			mb->release();
		}
		else
		{
			res = m_handler->handle_recv(mb);
		}
	}
	else
	{	
		mb->release();
	}

	release();
	
	return res;
}

int OOCore_Channel::close()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_closed)
		return -1;

	if (m_sibling != 0)
	{
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb,ACE_Message_Block(0,ACE_Message_Block::MB_HANGUP),-1);

		if (m_sibling->post_msg(mb,0) != 0)
		{
			mb->release();
			return -1;
		}		
	}

	if (m_handler != 0)
	{
		m_handler->handle_close();
		m_handler->release();
		m_handler = 0;
	}

	m_closed = true;

	guard.release();

	release();

	return 0;
}