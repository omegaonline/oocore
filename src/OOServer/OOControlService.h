#ifndef _OOSERVER_OOCONTROLSERVICE_H_INCLUDED_
#define _OOSERVER_OOCONTROLSERVICE_H_INCLUDED_

#include <list>

#include <ace/Singleton.h>

#include "../OOSvc/Channel_Acceptor.h"

#include "./OOControlClient.h"

class OOControlService : 
	public OOSvc_Object_Acceptor<OOObj::Client_Service,OOControlClient::Create>
{
public:
	OOControlService();
	virtual ~OOControlService();

	ACE_UINT16 stop(bool force);
	bool stop_pending();
	void stay_alive();

	void add_client(OOControlClient* client);
	void remove_client(OOControlClient* client);

private:
	ACE_Thread_Mutex m_lock;
	std::list<OOControlClient*> m_client_list;
	bool m_stayalive;
};

typedef ACE_Singleton<OOControlService, ACE_Thread_Mutex> OOCONTROL_SERVICE;

#endif // _OOSERVER_OOCONTROLSERVICE_H_INCLUDED_
