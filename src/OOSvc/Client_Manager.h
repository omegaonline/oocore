#pragma once

#include <ace/Acceptor.h>
#include <ace/MEM_Acceptor.h>

#include "../OOCore/Binding.h"

#include "./Shutdown.h"
#include "./Client_Connection.h"

class OOSvc_Client_Manager : 
	public ACE_Acceptor<OOSvc_Client_Connection, ACE_MEM_ACCEPTOR>,
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
