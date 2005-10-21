#pragma once

#include <ace/Event_Handler.h>

#include "./Channel.h"

#include "./OOCore_export.h"

// TODO This should not be an event_handler, it should be using OOCore_PostRequest

class OOCore_Export OOCore_Channel_Handler : public ACE_Event_Handler
{
	friend class OOCore_Channel;

public:
	OOCore_Channel_Handler(OOCore_Channel* channel);

	OOCore_Channel* channel(void)
	{
		return m_channel;
	}

	void addref()
	{
		++m_refcount;
	}

	void release()
	{
		if (--m_refcount==0)
			delete this;
	}

	bool closed()
	{
		return m_closed;
	}

protected:
	virtual ~OOCore_Channel_Handler() {};

	virtual int handle_recv(ACE_Time_Value* wait = 0)
	{
		return -1;
	}

	virtual int handle_close()
	{
		m_closed = true;
		m_channel = 0;
		return 0;
	}

private:
	bool m_closed;
	OOCore_Channel* m_channel;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;

	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
	{
		addref();

		// Just call handle_recv
		int ret = handle_recv();
		if (ret==0)
			release();

		return ret;
	}

	int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask close_mask)
	{
		release();

		return 0;
	}
};
