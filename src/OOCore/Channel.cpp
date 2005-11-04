#include "./Channel.h"

#include "./Channel_Handler.h"
#include "./Engine.h"

OOCore_Channel::OOCore_Channel() :
	m_sibling(0),
	m_handler(0),
	m_refcount(1),
	m_close_flags(NOT_CLOSED)
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
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Trying to bind channel handler at least twice\n")),-1);

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
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	if (m_close_flags & SEND_CLOSED)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Send on closed channel\n")),-1);

	return m_sibling->post_msg(mb,wait);
}

int OOCore_Channel::close()
{
	return close_i(false);
}

int OOCore_Channel::close_i(bool bRecv)
{
	ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

	// If we are not closed for send, send a HANGUP message
	if ((m_close_flags & SEND_CLOSED) == 0)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Channel %@ send close\n"),this));

		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb,ACE_Message_Block(0,ACE_Message_Block::MB_HANGUP),-1);

		if (m_sibling->post_msg(mb,0) != 0)
		{
			mb->release();
			return -1;
		}

		// Remember we have sent this
		m_close_flags |= SEND_CLOSED;
	}

	if (bRecv && (m_close_flags & RECV_CLOSED)==0)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Channel %@ recv close\n"),this));

		// Remember we have recieved this
		m_close_flags |= RECV_CLOSED;

		// tell the handler we are closed
		if (m_handler != 0)
		{
			OOCore_Channel_Handler* h = m_handler;
			m_handler = 0;

			guard.release();

			h->handle_close();
			h->release();

			guard.acquire();			
		}
	}

	if (m_close_flags == READY_TO_CLOSE)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Channel %@ all done\n"),this));

		m_close_flags |= CLOSED;

		if (--m_refcount==0)
			delete this;
	}

	return 0;
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

		if (ENGINE::instance()->post_request(p,wait)!=0)
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
	if ((m_close_flags & RECV_CLOSED) == 0)
	{
		if (mb->msg_type() == ACE_Message_Block::MB_HANGUP)
		{
			mb->release();

			res = close_i(true);
		}
		else 
		{
			res = m_handler->handle_recv(mb);
		}
	}
	else
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Recv'd on closed channel!\n")));

		mb->release();
	}

	if (--m_refcount==0)
		delete this;

	return res;
}
