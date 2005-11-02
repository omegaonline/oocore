#include "./Channel.h"

#include <ace/Reactor_Notification_Strategy.h>
#include <ace/Reactor.h>

#include "./Channel_Handler.h"
#include "./OOCore.h"

OOCore_Channel::OOCore_Channel() :
	m_sibling(0),
	m_handler(0),
	m_refcount(1),
	m_closing(false)
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
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_handler != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Trying to bind NULL channel handler\n")),-1);

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
	OOCore_Channel* s = m_sibling;

	if (s==0)
		return -1;

	return s->post_msg(mb,wait);
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
			--m_refcount;
			delete p;
			return -1;
		}
	}
	else
	{
		if (m_msg_queue.enqueue_tail(mb,wait)==-1)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed enqueue channel message\n")),-1);
	}
	
	return 0;
}

int OOCore_Channel::recv_i(ACE_Message_Block* mb)
{
	int res = -1;
	if (mb->msg_type() == ACE_Message_Block::MB_STOP)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Channel %@ stop\n"),this));

		res = close_i();
		mb->release();
	}
	else if (mb->msg_type() == ACE_Message_Block::MB_HANGUP)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Channel %@ hangup\n"),this));

		res = close_handler();
		mb->release();
	}
	else
	{
		OOCore_Channel_Handler* h = m_handler;
		if (h)
		{
			res = h->handle_recv(mb);
		}
		else
		{
			mb->release();
		}
	}

	if (--m_refcount==0)
		delete this;

	return res;
}

int OOCore_Channel::close_i()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_sibling!=0)
	{
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb,ACE_Message_Block(0,ACE_Message_Block::MB_HANGUP),-1);

		if (m_sibling->post_msg(mb,0) != 0)
		{
			mb->release();
			return -1;
		}
		
		m_sibling->m_closing = true;
		m_sibling = 0;
	}
	else
		ACE_ERROR((LM_DEBUG,ACE_TEXT("(%P|%t) Attempting to close channel twice\n")));

	guard.release();

	return close_handler();
}

int OOCore_Channel::close_handler()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_handler != 0)
	{
		m_handler->handle_close();
		m_handler->release();
		m_handler = 0;
	}

	guard.release();

	if (--m_refcount==0)
		delete this;

	return 0;
}

int OOCore_Channel::close()
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_sibling && !m_closing)
	{
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb,ACE_Message_Block(0,ACE_Message_Block::MB_STOP),-1);

		msg_param* p;
		ACE_NEW_NORETURN(p,msg_param);
		if (p==0)
		{
			mb->release();
			return -1;
		}

		p->mb = mb;
		p->pt = this;

		++m_refcount;

		guard.release();

		if (OOCore_PostCloseRequest(p) != 0)
		{
			--m_refcount;
			p->mb->release();
			delete p;
			return -1;
		}
	}

	return 0;
}