#pragma once

#include <ace/MEM_Stream.h>

#include "../OOCore/Client_Service.h"
#include "../OOCore/Transport_Svc_Handler.h"

#include "./Transport_Acceptor.h"
#include "./Shutdown.h"

class OOSvc_Client_Connection : 
	public OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER>,
	public OOSvc_Shutdown_Observer
{
	typedef OOCore_Transport_Svc_Handler<OOSvc_Transport_Acceptor,ACE_MEM_STREAM,ACE_MEM_STREAM_MIN_BUFFER> svc_base;

protected:
	bool is_local_transport()
	{
		return true;
	}

private:
	void handle_shutdown()
	{
		svc_base::close();
	}
};

class OOSvc_Client_Service : 
	public OOObj::Object_Impl<OOObj::Client_Service>
{
// OOObj::Object interface
public:
	int QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal);

// OOObj::Client_Service interface
public:
	int CreateObject(const OOObj::char_t* object, const OOObj::GUID& iid, OOObj::Object** ppVal);
	int Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray);
	int Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray);
	int Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray);
};
