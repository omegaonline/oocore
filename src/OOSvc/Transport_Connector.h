#pragma once

#include "../OOCore/Transport_Connector.h"

#include "./OOSvc_export.h"

class OOSvc_Export OOSvc_Transport_Connector : 
	public OOCore_Transport_Connector,
	public OOCore_Transport_Service
{
protected:
	OOSvc_Transport_Connector();
	virtual ~OOSvc_Transport_Connector(void) {};

	virtual int open();

private:
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;

// OOObj::Object interface
public:
	int AddRef();
	int Release();
	int QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal);

// OOCore_Transport_Service
public:
	int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key);
	int SetReverse(OOCore_Transport_Service* reverse);
};
