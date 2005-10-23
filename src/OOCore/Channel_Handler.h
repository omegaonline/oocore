#pragma once

#include <ace/Event_Handler.h>

#include "./Channel.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_Channel_Handler
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

	virtual int handle_recv(ACE_Message_Block* mb)
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
};
