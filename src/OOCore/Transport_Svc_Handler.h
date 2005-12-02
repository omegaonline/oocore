#ifndef OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_
#define OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_

#include <ace/Svc_Handler.h>

#include "./Transport_Impl.h"

namespace OOCore
{

template <const bool bAcceptor, ACE_PEER_STREAM_1, const int Buffer_Size>
class Transport_Svc_Handler :
	public ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH>,
	public Transport_Impl
{
	typedef ACE_Svc_Handler<ACE_PEER_STREAM_2, ACE_MT_SYNCH> svc_class;

public:
	Transport_Svc_Handler() : m_bOpen(false)
	{ }

	virtual int open(void* p = 0)
	{
		if (svc_class::open(p)!=0)
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Service handler open failed\n")),-1);

		if (open_transport(bAcceptor)!=0)
			return -1;

		m_bOpen = true;

		// Raise our ref count while we are open
		AddRef();

		return 0;
	}

	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		return handle_recv();
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
			return 0;
		
		if (m_bOpen)
		{
			if (close_transport() != 0)
				return -1;

			// Release our own ref count - we are closed
			Release();
		}

		// Do not call svc_class::handle_close() it calls delete!
		return 0;
	}

protected:
	int send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0)
	{
		//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) transport sends %d bytes\n"),mb->length()));

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

	int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0)
	{
        ACE_NEW_RETURN(mb,ACE_Message_Block(Buffer_Size),-1);

		// Recv some data
		ssize_t recv_cnt = this->peer().recv(mb->wr_ptr(),Buffer_Size);
		
		// See if we got anything
		if (recv_cnt <= 0)
		{
			// Connection closed
			mb->release();
			return -1;
		}

		// Set the wr_ptr to the end
		mb->wr_ptr(recv_cnt);

		return 0;
	}

	virtual ssize_t send_n(ACE_Message_Block* mb) = 0;

private:
	bool m_bOpen;

	int send_i()
	{
		ACE_Message_Block *mb;
		ACE_Time_Value nowait(ACE_OS::gettimeofday());
		while (-1 != getq(mb, &nowait))
		{
			// Send the data
			ssize_t send_cnt = send_n(mb);
			
			//ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) socket actually sends %d bytes\n"),send_cnt));

			if (send_cnt == -1 && ACE_OS::last_error() != EWOULDBLOCK)
				ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) %p\n"),ACE_TEXT("send")),-1);
			
			if (send_cnt == -1)
				send_cnt = 0;
			
			if (mb->total_length() > static_cast<size_t>(send_cnt))
			{
				ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) SPARE!!\n")));
				ungetq(mb);
				return 0;
			}
			
			mb->release();
		}

		return 0;
	}
};

};

#endif // OOCORE_TRANSPORT_SVC_HANDLER_H_INCLUDED_
