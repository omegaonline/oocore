#ifndef _OOSVC_TRANSPORT_ACCEPTOR_H_INCLUDED_
#define _OOSVC_TRANSPORT_ACCEPTOR_H_INCLUDED_

#include "../OOCore/Transport_Base.h"
#include "../OOCore/Transport_Service.h"

#include "./OOSvc_export.h"

class OOSvc_Export OOSvc_Transport_Acceptor :
	public OOCore_Transport_Base,
	public OOCore_Transport_Service
{
protected:
	OOSvc_Transport_Acceptor(void);
	virtual ~OOSvc_Transport_Acceptor(void) {};

	int open();
	
	virtual int request_close();

	virtual bool is_local_transport();
		
private:
	ACE_Active_Map_Manager<OOCore_Channel*> m_channel_map;
	ACE_RW_Thread_Mutex m_lock;
	OOCore_Transport_Service* m_interface;
	ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;

	int find_channel(const ACE_Active_Map_Manager_Key& key, OOCore_Channel*& channel);
	int bind_channel(OOCore_Channel* channel, ACE_Active_Map_Manager_Key& key);
	int unbind_channel(const ACE_Active_Map_Manager_Key& key);
	int close_all_channels();
	int connect_channel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key& key, OOCore_Channel** channel);

	static bool await_close(void* p);

// OOObj::Object interface
public:
	int AddRef();
	int Release();
	int QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal);

// OOCore_Transport_Service
public:
	int OpenChannel(const OOObj::char_t* name, ACE_Active_Map_Manager_Key* channel_key);
	int CloseChannel(OOObj::cookie_t channel_key);
	int SetReverse(OOCore_Transport_Service* reverse);
};

#endif // _OOSVC_TRANSPORT_ACCEPTOR_H_INCLUDED_
