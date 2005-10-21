#pragma once

#include <ace/Svc_Handler.h>
#include <ace/Message_Queue.h>
#include <ace/Reactor_Notification_Strategy.h>

#include "./Transport_Base.h"

// TODO This should not using a ACE_Message_Queue, it should be using OOCore_PostRequest

template <class Transport, ACE_PEER_STREAM_1, const int Buffer_Size>
class OOCore_Transport_Svc_Handler :
	public ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH>,
	public Transport
{
	typedef ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH> svc_class;

public:
	OOCore_Transport_Svc_Handler()
	{
		ACE_Reactor_Notification_Strategy* strategy = 0;
		ACE_NEW(strategy,ACE_Reactor_Notification_Strategy(ACE_Reactor::instance(),this,ACE_Event_Handler::EXCEPT_MASK));
		m_msg_queue.notification_strategy(strategy);
	}

	virtual ~OOCore_Transport_Svc_Handler()
	{
		delete m_msg_queue.notification_strategy();
	}

	virtual int open(void* p = 0)
	{
		if (svc_class::open(p)!=0)
			return -1;

		if (Transport::open()!=0)
			return -1;

		return 0;
	}

	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		ACE_Message_Block* mb;
		ACE_NEW_RETURN(mb,ACE_Message_Block(Buffer_Size),-1);
			
		// Recv some data
		ssize_t recv_cnt = this->peer().recv(mb->wr_ptr(),Buffer_Size);
		if (recv_cnt <= 0)
		{
			// Connection closed
			mb->release();
			return -1;
		}

		mb->wr_ptr(recv_cnt);

		return (m_msg_queue.enqueue_tail(mb) == -1 ? -1 : 0);
	}

	int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		if (send_i() != 0)
			return -1;

		return (this->msg_queue()->is_empty()) ? -1 : 0;
	}

	int handle_exception(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		// We use this to handle internal message posting
		ACE_Message_Block* mb;
		if (m_msg_queue.dequeue_head(mb) == -1)
			return -1;

		return Transport::recv(mb);
	}

	virtual int handle_close(ACE_HANDLE fd = ACE_INVALID_HANDLE, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK)
	{
		if (mask == ACE_Event_Handler::WRITE_MASK)
		{
			return 0;
		}

		if (close_transport() != 0)
			return -1;

		return svc_class::handle_close(fd,mask);
	}

protected:
	int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0)
	{
		if (this->putq(mb,wait) == -1)
		{
			ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) %p; discarding data\n"),ACE_TEXT("enqueue failed")));
			return -1;
		}

		if (send_i() != 0)
			return -1;
		
		if (!this->msg_queue()->is_empty())
		{
			// Wait for a WRITE_EVENT
			return this->reactor()->register_handler(this,ACE_Event_Handler::WRITE_MASK);
		}

		return 0;
	}

private:
	ACE_Thread_Mutex m_lock;
	ACE_Message_Queue<ACE_MT_SYNCH> m_msg_queue;

	int send_i()
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

		ACE_Message_Block *mb,*mb_start;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != getq(mb_start, &nowait))
		{
			for (mb=mb_start;mb!=0;mb=mb->cont())
			{
				if (mb->length()==0)
					continue;

				ssize_t send_cnt = this->peer().send(mb->rd_ptr(), mb->length());
				if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
					ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) %p\n"),ACE_TEXT("send")),-1);
				else
				{
					if (send_cnt == -1)
					{
						send_cnt = 0;
					}
					mb->rd_ptr(static_cast<size_t>(send_cnt));
				}

				if (mb->length() > 0)
				{
					ungetq(mb_start);
					return 0;
				}
			}
			mb_start->release();
		}

		return 0;
	}
};
