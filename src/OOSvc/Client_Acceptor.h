#pragma once

#include <ace/MEM_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"

#include "./Transport_Acceptor.h"
#include "./Shutdown.h"

class OOSvc_Client_Acceptor : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>,
	public OOSvc_Shutdown_Observer
{
protected:
	bool is_local_transport();

private:
	void handle_shutdown();
};
