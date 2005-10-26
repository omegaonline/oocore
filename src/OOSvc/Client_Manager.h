#ifndef _OOSVC_CLIENT_MANAGER_H_INCLUDED_
#define _OOSVC_CLIENT_MANAGER_H_INCLUDED_

#include <ace/Acceptor.h>
#include <ace/MEM_Acceptor.h>

#include "./Shutdown.h"
#include "./Client_Acceptor.h"

class OOSvc_Client_Manager : 
	public ACE_Acceptor<OOSvc_Client_Acceptor, ACE_MEM_ACCEPTOR>,
	public OOSvc_Shutdown_Observer
{
public:
	OOSvc_Client_Manager(void);
	virtual ~OOSvc_Client_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

private:
	void handle_shutdown();
};

#endif // _OOSVC_CLIENT_MANAGER_H_INCLUDED_
