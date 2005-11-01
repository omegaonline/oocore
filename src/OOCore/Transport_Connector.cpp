#include "./Transport_Connector.h"

#include "./ProxyStub_Handler.h"
#include "./Channel.h"

OOCore_Transport_Connector::OOCore_Transport_Connector(void) :
	m_interface(0)
{
}

OOCore_Transport_Connector::~OOCore_Transport_Connector(void) 
{
	close();
};

int OOCore_Transport_Connector::open()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Transport %@ open\n"),this));

	OOCore_Channel* channel;
	if (connect_primary_channel(&channel) != 0)
		return -1;

	// Create a proxy handler
	OOCore_ProxyStub_Handler* ph;
	ACE_NEW_NORETURN(ph,OOCore_ProxyStub_Handler(channel));
	if (ph == 0)
	{
		channel->close();
		return -1;
	}

	// Try to create the first proxy on it
	if (ph->create_first_proxy(reinterpret_cast<OOObj::Object**>(&m_interface)) != 0)
	{
		channel->close();
		return -1;
	}

	if (m_interface->SetReverse(this) != 0)
	{
		m_interface->Release();
		m_interface = 0;
		channel->close();
		return -1;
	}

	addref();

	return 0;
}

int OOCore_Transport_Connector::close()
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	OOCore_Transport_Service* i = m_interface;

	m_interface = 0;

	guard.release();

	if (i != 0)
		i->Release();
	
	return close_transport();
}

int OOCore_Transport_Connector::find_channel(const ACE_Active_Map_Manager_Key& key, OOCore_Channel*& channel)
{
	ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	map_type::iterator i = m_channel_map.find(key);
	if (i==m_channel_map.end())
		return -1;
	
	channel = i->second;

	return 0;
}

int OOCore_Transport_Connector::bind_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key)
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	if (!m_channel_map.insert(map_type::value_type(key,channel)).second)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to insert channel key\n")),-1);
		
	return 0;
}

int OOCore_Transport_Connector::unbind_channel(const ACE_Active_Map_Manager_Key& key)
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	if (m_channel_map.erase(key) == 1)
		return 0;
	
	ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to unbind channel key\n")),-1);
}

int OOCore_Transport_Connector::close_all_channels()
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	for (map_type::iterator i=m_channel_map.begin();i!=m_channel_map.end();++i)
	{
		i->second->close();
	}
	
	return 0;
}

int OOCore_Transport_Connector::connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel)
{
	if (!m_interface)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connecting channel with no transport interface\n")),-1);

	if (m_interface->OpenChannel(name,&key) != 0)
		return -1;

	if (accept_channel(*channel,key) != 0)
		return -1;
	
	return 0;
}

int OOCore_Transport_Connector::CloseChannel(OOObj::cookie_t channel_key)
{
	ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	map_type::iterator i = m_channel_map.find(channel_key);
	if (i==m_channel_map.end())
		return -1;

	OOCore_Channel* ch = i->second;

	guard.release();
	
	return ch->close();
}

int OOCore_Transport_Connector::OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key)
{
	// No! We are a uni-directional
	return -1;
}

int OOCore_Transport_Connector::SetReverse(OOCore_Transport_Service* reverse)
{
	// No! We are a connector
	return -1;
}
