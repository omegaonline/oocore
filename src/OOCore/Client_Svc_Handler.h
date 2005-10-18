#pragma once

#include <ace/MEM_Stream.h>

#include "./Transport_Svc_Handler.h"

template <class Transport>
class OOCore_Client_Svc_Handler :
	public OOCore_Transport_Svc_Handler<Transport,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>
{
};