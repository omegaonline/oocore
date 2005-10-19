#pragma once

#include <ace/MEM_Stream.h>

#include "./Protocol_Acceptor.h"

class OOSvc_Client_Acceptor : 
	public OOSvc_Protocol_Acceptor<ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
protected:
	bool is_local_transport();
};
