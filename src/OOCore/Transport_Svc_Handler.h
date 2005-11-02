#ifndef _OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_
#define _OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_

#include <ace/Svc_Handler.h>
#include <ace/Message_Queue.h>
#include <ace/Reactor_Notification_Strategy.h>

#include "./Transport_Base.h"

template <class Transport, ACE_PEER_STREAM_1, const int Buffer_Size>
class OOCore_Transport_Svc_Handler :
	public ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH>,
	public Transport
{
	typedef ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH> svc_class;

public:
	virtual int open(void* p = 0)
	{
		if (svc_class::open(p)!=0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Service handler open failed\n")),-1);

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

		RecvRequest* req = 0;
		ACE_NEW_NORETURN(req,RecvRequest(this,mb));
		if (req==0)
		{
			mb->release();
			return -1;
		}	

		return (OOCore_PostRequest(req) == -1 ? -1 : 0);
	}

	int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		if (send_i() != 0)
			return -1;

		return (this->msg_queue()->is_empty()) ? -1 : 0;
	}

	int handle_close(ACE_HANDLE fd = ACE_INVALID_HANDLE, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK)
	{
		if (mask == ACE_Event_Handler::WRITE_MASK)
		{
			return 0;
		}

		return close_transport();
	}

protected:
	int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0)
	{
		if (this->putq(mb,wait) == -1)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) %p; discarding data\n"),ACE_TEXT("enqueue failed")),-1);
		
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

	class RecvRequest : public ACE_Method_Request
	{
	public:
		RecvRequest(OOCore_Transport_Svc_Handler<Transport,ACE_PEER_STREAM_2,Buffer_Size>* p, ACE_Message_Block* m) : 
		  pt(p), mb(m)
		{}

		int call()
		{
			return pt->recv_i(mb);
		}

		OOCore_Transport_Svc_Handler<Transport,ACE_PEER_STREAM_2,Buffer_Size>* pt;
		ACE_Message_Block* mb;
	};
	friend class RecvRequest;
	
	int send_i()
	{
		ACE_Guard<ACE_Thread_Mutex> guard(m_lock);

		ACE_Message_Block *mb,*mb_start;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != getq(mb_start, &nowait))
		{
			// TODO make this use peer()->send_n() instead!

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

	int recv_i(ACE_Message_Block* mb)
	{
		// We use this to handle internal message posting
		if (Transport::recv(mb) != 0)
		{
			svc_class::close();
			return -1;
		}

		return 0;
	}
};

#endif // _OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_
