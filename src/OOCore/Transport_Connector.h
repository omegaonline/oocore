#ifndef _OOCORE_TRANSPORT_CONNECTOR_H_INCLUDED_
#define _OOCORE_TRANSPORT_CONNECTOR_H_INCLUDED_

#include <list>
#include <map>

#include <ace/RW_Thread_Mutex.h>

#include "./Transport_Base.h"
#include "./Transport_Service.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_Transport_Connector :
	public OOCore_Transport_Base,
	public OOCore_Transport_Service
{
protected:
	OOCore_Transport_Connector(void);
	virtual ~OOCore_Transport_Connector(void);

	int open();
	int close();
		
private:
	typedef std::map<ACE_Active_Map_Manager_Key,OOCore_Channel*> map_type;

	OOCore_Transport_Service* m_interface;
	map_type m_channel_map;
	ACE_RW_Thread_Mutex m_lock;
			
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
	virtual int OpenChannel(const OOObj::char_t* name, OOObj::cookie_t* channel_key);
	int CloseChannel(OOObj::cookie_t channel_key);
	int SetReverse(OOCore_Transport_Service* reverse);
};

#endif // _OOCORE_TRANSPORT_CONNECTOR_H_INCLUDED_
