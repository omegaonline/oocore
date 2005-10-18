#pragma once

#include <ace/Task.h>
#include <ace/Acceptor.h>
#include <ace/SOCK_Acceptor.h>

#include "../OOCore/Binding.h"
#include "../OOSvc/Shutdown.h"
#include "../OOSvc/Transport_Protocol.h"

#include "./TcpIp_Acceptor.h"

class OONet_TcpIp_Manager : 
	public ACE_Task<ACE_MT_SYNCH>,
	public OOSvc_Shutdown_Observer,
	public OOSvc_Transport_Protocol
{
	typedef ACE_Acceptor<OONet_TcpIp_Acceptor, ACE_SOCK_ACCEPTOR> TcpIpAcceptor;

public:
	OONet_TcpIp_Manager(void);
	virtual ~OONet_TcpIp_Manager(void);

	int init(int argc, ACE_TCHAR *argv[]);
	int fini(void);

private:
	TcpIpAcceptor m_acceptor;

	void handle_shutdown();
	bool AddressIsEqual(const char* addr1, const char* addr2);
};
