#pragma once

#include <ace/SOCK_Stream.h>

#include "../OOSvc/Protocol_Acceptor.h"

class OONet_TcpIp_Acceptor : 
	public OOSvc_Protocol_Acceptor<ACE_SOCK_STREAM,2048>
{
public:
	
};
