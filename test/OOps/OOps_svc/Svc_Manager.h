#pragma once

#include <ace/Task.h>

#include "../../src/OOSvc/Shutdown.h"
#include "../../src/OOSvc/Channel_Acceptor.h"

#include "./Local_Service_Impl.h"
#include "./Remote_Service_Impl.h"

#include "./OOps_export.h"

class OOps_Export OOps_Svc_Manager : 
	public ACE_Task<ACE_MT_SYNCH>,
	public OOSvc_Shutdown_Observer
{
public:
	OOps_Svc_Manager(void);
	virtual ~OOps_Svc_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

private:
	OOSvc_Object_Acceptor<OOps_Local_Service,OOps_Local_Service_Impl::Create> m_Local_Service;
	OOSvc_Object_Acceptor<OOps_Remote_Service,OOps_Remote_Service_Impl::Create> m_Remote_Service;

	void handle_shutdown();
};
