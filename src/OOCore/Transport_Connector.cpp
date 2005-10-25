#include "./Transport_Connector.h"

#include "./ProxyStub_Handler.h"
#include "./Channel.h"

OOCore_Transport_Connector::OOCore_Transport_Connector(void) :
	m_interface(0), m_closing(false)
{
}

OOCore_Transport_Connector::~OOCore_Transport_Connector(void) 
{
	if (m_interface != 0)
		m_interface->Release();
};

int OOCore_Transport_Connector::open()
{
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

	addref();

	return 0;
}

OOCore_Transport_Service* OOCore_Transport_Connector::get_interface()
{
	return m_interface;
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

	return (m_channel_map.insert(map_type::value_type(key,channel)).second ? 0 : -1);
}

int OOCore_Transport_Connector::unbind_channel(const ACE_Active_Map_Manager_Key& key)
{
	if (m_closing)
		return 0;

	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	return (m_channel_map.erase(key) == 1 ? 0 : -1);
}

int OOCore_Transport_Connector::close_all_channels()
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	m_closing = true;

	for (map_type::iterator i=m_channel_map.begin();i!=m_channel_map.end();++i)
	{
		i->second->close();
	}
	m_channel_map.clear();

	m_closing = false;

	return 0;
}

int OOCore_Transport_Connector::connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel)
{
	if (!m_interface)
		return -1;

	if (m_interface->OpenChannel(name,&key) != 0)
		return -1;

	if (accept_channel(*channel,key) != 0)
		return -1;
	
	return 0;

}
