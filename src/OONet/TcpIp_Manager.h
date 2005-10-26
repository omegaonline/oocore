#ifndef _OONET_TCPIP_MANAGER_H_INCLUDED_
#define _OONET_TCPIP_MANAGER_H_INCLUDED_

#include <ace/SOCK_Acceptor.h>

#include "../OOSvc/Transport_Protocol.h"

#include "./TcpIp_Acceptor.h"

class OONet_TcpIp_Manager : 
	public ACE_Acceptor<OONet_TcpIp_Acceptor, ACE_SOCK_ACCEPTOR>,
	public OOSvc_Shutdown_Observer,
	public OOSvc_Transport_Protocol
{
public:
	OONet_TcpIp_Manager(void);
	virtual ~OONet_TcpIp_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

private:
	bool address_is_equal(const char* addr1, const char* addr2);
	int connect_transport(const char* remote_host, OOCore_Transport_Base*& transport);
	void handle_shutdown();
};

#endif // _OONET_TCPIP_MANAGER_H_INCLUDED_
