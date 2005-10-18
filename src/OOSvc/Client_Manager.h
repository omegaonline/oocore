#pragma once

#include <ace/Task.h>
#include <ace/Acceptor.h>
#include <ace/MEM_Acceptor.h>

#include "../OOCore/Binding.h"

#include "./Shutdown.h"
#include "./Client_Connection.h"

class OOSvc_Client_Manager : 
	public ACE_Task<ACE_MT_SYNCH>,
	public OOSvc_Shutdown_Observer
{
	typedef ACE_Acceptor<OOSvc_Client_Connection, ACE_MEM_ACCEPTOR> ClientAcceptor;

public:
	OOSvc_Client_Manager(void);
	virtual ~OOSvc_Client_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

private:
	ClientAcceptor m_acceptor;

	void handle_shutdown();
};
