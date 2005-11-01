#include "./Transport_Connector.h"

#include "./Service_Manager.h"

OOSvc_Transport_Connector::OOSvc_Transport_Connector()
{
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


