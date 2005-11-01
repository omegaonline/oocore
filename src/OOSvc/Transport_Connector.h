#ifndef _OOSVC_TRANSPORT_CONNECTOR_H_INCLUDED_
#define _OOSVC_TRANSPORT_CONNECTOR_H_INCLUDED_

#include "../OOCore/Transport_Connector.h"

#include "./OOSvc_export.h"

// This class is the bi-directional implementation of
// OOCore_Transport_Connector

class OOSvc_Export OOSvc_Transport_Connector : 
	public OOCore_Transport_Connector
{
protected:
	OOSvc_Transport_Connector();
	virtual ~OOSvc_Transport_Connector(void) {};

// OOCore_Transport_Service
public:
	int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key);
};

#endif // _OOSVC_TRANSPORT_CONNECTOR_H_INCLUDED_
