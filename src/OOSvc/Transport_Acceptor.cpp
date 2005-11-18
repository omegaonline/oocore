#include "./Transport_Acceptor.h"

#include <ace/Reactor.h>
#include <ace/Countdown_Time.h>

#include "../OOCore/Channel.h"
#include "../OOCore/Engine.h"

#include "./Service_Manager.h"

OOSvc_Transport_Acceptor::OOSvc_Transport_Acceptor(void)
{
}

int OOSvc_Transport_Acceptor::open()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("(%P|%t) Transport %@ open\n"),this));

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
		channel->close();
		return -1;
	}

	return 0;
}

bool OOSvc_Transport_Acceptor::await_close(void* p)
{
	OOSvc_Transport_Acceptor* pThis = static_cast<OOSvc_Transport_Acceptor*>(p);

	ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(pThis->m_lock);

	if (pThis->m_channel_map.current_size()==0)
		return true;

	return false;
}

int OOSvc_Transport_Acceptor::request_close()
{
	/*ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	OOCore_Transport_Service* i = m_interface;
	m_interface = 0;

	guard.release();

	if (i != 0)
		i->Release();

	ACE_Time_Value wait(5);
	ENGINE::instance()->pump_requests(&wait,await_close,this);*/
	
	return close_transport(false);
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
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);
	
	if (m_channel_map.unbind(key) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to unbind channel key\n")),-1);

	return 0;
}

int OOSvc_Transport_Acceptor::close_all_channels()
{
	ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_lock);

	for (ACE_Active_Map_Manager<OOCore_Channel*>::iterator i=m_channel_map.begin();i!=m_channel_map.end();++i)
	{
		(*i).int_id_->close();
	}
	m_channel_map.unbind_all();
	
	return 0;
}

bool OOSvc_Transport_Acceptor::is_local_transport()
{
	return false;
}

int OOSvc_Transport_Acceptor::connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel)
{
	OOObj::Object_Ptr<OOCore_Transport_Service> inter = interface();
	
	if (!inter)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Connecting channel with no transport interface\n")),-1);
	
	// Add a channel to ourselves
	if (accept_channel(*channel,key) != 0)
		return -1;
	
	if (inter->OpenChannel(name,&key) != 0)
	{
		(*channel)->close();
		return -1;
	}

	return 0;
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
		channel->close();
		return -1;
	}

	return 0;
}

int OOSvc_Transport_Acceptor::Connect(OOCore_Transport_Service* reverse)
{
	return interface(reverse);
}
