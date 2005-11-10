#include "./Client_Acceptor.h"

bool OOSvc_Client_Acceptor::is_local_transport()
{
	return true;
}

void OOSvc_Client_Acceptor::handle_shutdown()
{
	request_close();
}

int OOSvc_Client_Acceptor::request_close()
{
	shutdown();

	return OOSvc_Transport_Acceptor::request_close();
}
