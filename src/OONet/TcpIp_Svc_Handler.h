#pragma once

#include <ace/SOCK_Stream.h>

#include "../OOCore/Transport_Svc_Handler.h"

template <class Transport>
class OONet_TcpIp_Svc_Handler :
	public OOCore_Transport_Svc_Handler<Transport,ACE_SOCK_STREAM,2048>
{
};
