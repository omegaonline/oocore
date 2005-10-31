#include "./Transport_Connector.h"

#include "./Service_Manager.h"

OOSvc_Transport_Connector::OOSvc_Transport_Connector()
	: m_refcount(0)
{
}

int OOSvc_Transport_Connector::open(void)
{
	int ret = OOCore_Transport_Connector::open();
	if (ret == 0)
	{
		ret = get_interface()->SetReverse(this);
		if (ret!=0)
			close_transport();
	}

	return ret;
}

int OOSvc_Transport_Connector::AddRef()
{
	return addref();
}

int OOSvc_Transport_Connector::Release()
{
	return release();
}

int OOSvc_Transport_Connector::QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
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

int OOSvc_Transport_Connector::OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key)
{
	// Add a channel to ourselves
	OOCore_Channel* channel;
	if (accept_channel(channel,*channel_key) != 0)
		return -1;
	
	// Ask the service manager to connect to the channel
	if (SERVICE_MANAGER::instance()->connect_service(name,false,channel) != 0)
	{
		channel->close();
		return -1;
	}

	return 0;
}

int OOSvc_Transport_Connector::SetReverse(OOCore_Transport_Service* reverse)
{
	// No! We are a connector
	return -1;
}
