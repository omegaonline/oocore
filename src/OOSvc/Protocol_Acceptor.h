#pragma once

#include "../OOCore/Transport_Svc_Handler.h"
#include "./Transport_Acceptor.h"
#include "./Shutdown.h"
#include "./Transport_Protocol.h"

template <ACE_PEER_STREAM_1, const int Buffer_Size>
class OOSvc_Protocol_Acceptor : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_PEER_STREAM_2,Buffer_Size>,
	public OOSvc_Shutdown_Observer
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_PEER_STREAM_2,Buffer_Size> acceptor_base;

public:
	int open(void* p)
	{
		m_protocol = reinterpret_cast<OOSvc_Transport_Protocol*>(p);
		
		return acceptor_base::open(p);
	}

private:
	virtual int handle_close(ACE_HANDLE fd = ACE_INVALID_HANDLE, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK)
	{
		if (m_protocol)
			m_protocol->transport_closed(this);

		return acceptor_base::handle_close(fd,mask);
	}

	void handle_shutdown()
	{
		close();
	}

	OOSvc_Transport_Protocol* m_protocol;
};
