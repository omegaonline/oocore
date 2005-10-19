#include "./Client_Connection.h"

bool OOSvc_Client_Connection::is_local_transport()
{
	return true;
}

void OOSvc_Client_Connection::handle_shutdown()
{
	close();
}