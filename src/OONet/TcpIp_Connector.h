#pragma once

#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "../OOCore/Object.h"
#include "../OOSvc/Transport_Manager.h"
#include "../OOSvc/Transport_Connector.h"

#include "./TcpIp_Svc_Handler.h"

class OONet_TcpIp_Connector :
    public OONet_TcpIp_Svc_Handler<OOSvc_Transport_Connector>
{
	typedef OONet_TcpIp_Svc_Handler<OOSvc_Transport_Connector> svc_base;

public:
};


