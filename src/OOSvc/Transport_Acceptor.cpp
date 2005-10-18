#include "./Transport_Acceptor.h"

#include <ace/Reactor.h>
#include <ace/Countdown_Time.h>

#include "../OOCore/Channel.h"

#include "./Service_Manager.h"

OOSvc_Transport_Acceptor::OOSvc_Transport_Acceptor(void) : 
	m_refcount(0), m_closing(false), m_interface(0)
{
}

int OOSvc_Transport_Acceptor::open()
{
	// Accept the first channel
	OOCore_Channel* channel;
	ACE_Active_Map_Manager_Key key;
	if (accept_channel(channel,key) != 0)
		return -1;

	// Create a stub handler
	OOCore_ProxyStub_Handler* sh;
	ACE_NEW_RETURN(sh,OOCore_ProxyStub_Handler(channel),-1);
	if (sh->create_first_stub(OOCore_Transport_Service::IID,this) != 0)
	{
		channel->close(true);
		return -1;
	}

	return 0;
}

int OOSvc_Transport_Acceptor::find_channel(const ACE_Active_Map_Manager_Key& key, OOCore_Channel*& channel)
{
	ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	return m_channel_map.find(key,channel);
}

int OOSvc_Transport_Acceptor::bind_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key)
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	return m_channel_map.bind(channel,key);
}

int OOSvc_Transport_Acceptor::unbind_channel(const ACE_Active_Map_Manager_Key& key)
{
	if (m_closing)
		return 0;

	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	return m_channel_map.unbind(key);
}

int OOSvc_Transport_Acceptor::close_all_channels()
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	m_closing = true;

	for (ACE_Active_Map_Manager<OOCore_Channel*>::iterator i=m_channel_map.begin();i!=m_channel_map.end();++i)
	{
		(*i).int_id_->close(true);
	}
	m_channel_map.close();

	m_closing = false;

	return (m_refcount==0 ? 0 : -1);
}

bool OOSvc_Transport_Acceptor::is_local_transport()
{
	return false;
}

int OOSvc_Transport_Acceptor::connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel)
{
	if (!m_interface)
		return -1;

	// Add a channel to ourselves
	if (accept_channel(*channel,key) != 0)
		return -1;
	
	if (m_interface->OpenChannel(name,&key) != 0)
	{
		(*channel)->close(false);
		return -1;
	}

	return 0;
}

int OOSvc_Transport_Acceptor::AddRef()
{
	++m_refcount;
	return 0;
}

int OOSvc_Transport_Acceptor::Release()
{
	if (--m_refcount < 0)
		return -1;

	return 0;
}

int OOSvc_Transport_Acceptor::QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	if (iid == OOCore_Transport_Service::IID ||
		iid == OOObj::Object::IID)
	{
		*ppVal = this;
		(*ppVal)->AddRef();
		return 0;
	}
	
	*ppVal = 0;
	
	return -1;
}

int OOSvc_Transport_Acceptor::OpenChannel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key* channel_key)
{
	// Add a channel to ourselves
	OOCore_Channel* channel;
	if (accept_channel(channel,*channel_key) != 0)
		return -1;
	
	// Ask the service manager to connect to the channel
	if (SERVICE_MANAGER::instance()->connect_service(name,is_local_transport(),channel) != 0)
	{
		channel->close(false);
		return -1;
	}

	return 0;
}

int OOSvc_Transport_Acceptor::SetReverse(OOCore_Transport_Service* reverse)
{
	if (m_interface==0)
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

		if (m_interface==0)
		{
			// Don't AddRef, its irrelevant
			m_interface = reverse;
			
			return 0;
		}
	}

	return -1;
}