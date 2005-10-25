#pragma once

#include "../OOCore/Transport_Base.h"
#include "../OOCore/Transport_Service.h"
#include "../OOCore/ProxyStub_Handler.h"

#include "./OOSvc_export.h"

class OOSvc_Export OOSvc_Transport_Acceptor :
	public OOCore_Transport_Base,
	public OOCore_Transport_Service
{
protected:
	OOSvc_Transport_Acceptor(void);
	virtual ~OOSvc_Transport_Acceptor(void) {};

	int open();

	virtual bool is_local_transport();
	
private:
	ACE_Active_Map_Manager<OOCore_Channel*> m_channel_map;
	ACE_RW_Thread_Mutex m_lock;
	bool m_closing;
	OOCore_Transport_Service* m_interface;

	int find_channel(const ACE_Active_Map_Manager_Key& key, OOCore_Channel*& channel);
	int bind_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key);
	int unbind_channel(const ACE_Active_Map_Manager_Key& key);
	int close_all_channels();
	int connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel);

// OOObj::Object interface
public:
	int AddRef();
	int Release();
	int QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal);

// OOCore_Transport_Service
public:
	int OpenChannel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key* channel_key);
	int SetReverse(OOCore_Transport_Service* reverse);
};
