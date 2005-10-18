#include "./Channel_Handler.h"

#include "./Channel.h"

OOCore_Channel_Handler::OOCore_Channel_Handler(OOCore_Channel* channel) :
	m_channel(channel), m_refcount(0), m_closed(false)
{
	m_channel->bind_handler(this);
}
