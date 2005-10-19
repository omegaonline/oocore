#include "./TcpIp_Manager.h"

#include <ace/Service_Config.h>
#include <ace/Get_Opt.h>
#include <ace/SOCK_Connector.h>
#include <ace/Connector.h>

#include "../OOCore/Binding.h"
#include "../OOSvc/Transport_Manager.h"

#include "./OONet_export.h"

// Declare the service
ACE_FACTORY_DEFINE(OONet,OONet_TcpIp_Manager)

OONet_TcpIp_Manager::OONet_TcpIp_Manager(void) :
	OOSvc_Transport_Protocol("tcp")
{
}

OONet_TcpIp_Manager::~OONet_TcpIp_Manager(void)
{
}

int OONet_TcpIp_Manager::init(int argc, ACE_TCHAR *argv[])
{
	if (ACE::debug())
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("TcpIp_Manager::init\n")));

	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":p:"),0);
	int option;
	u_short uPort = 0;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT('p'):
			uPort = ACE_OS::atoi(cmd_opts.opt_arg());
			break;

		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	ACE_INET_Addr port_addr(uPort);
	if (open(port_addr) == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	
	// Confirm we have a connection
	if (acceptor().get_local_addr(port_addr)==-1)
		return -1;

	if (TRANSPORT_MANAGER::instance()->register_protocol(this) != 0)
	{
		close();
		return -1;
	}
	
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening on port %u for TCP/IP connections.\n"),port_addr.get_port_number()));
			
	return 0;
}

void OONet_TcpIp_Manager::handle_shutdown()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OONet_TcpIp_Manager::shutdown\n")));

	close();
}

int OONet_TcpIp_Manager::fini(void)
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OONet_TcpIp_Manager::fini\n")));

	TRANSPORT_MANAGER::instance()->unregister_protocol(protocol_name());

	return 0;
}

bool OONet_TcpIp_Manager::address_is_equal(const char* addr1, const char* addr2)
{
	return ACE_INET_Addr(addr1) == ACE_INET_Addr(addr2);
}

int OONet_TcpIp_Manager::connect_transport(const char* remote_host, OOCore_Transport_Base*& transport)
{
	/*// Sort out address
	ACE_INET_Addr addr;
	if (addr.set(connect_string) != 0)
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("Bad tcp address format: %s"),connect_string),-1);

	// Connect to this
	OONet_TcpIp_Connector* pThis = this;
	ACE_Connector<OONet_TcpIp_Connector, ACE_SOCK_CONNECTOR> connector;
	if (connector.connect(pThis,addr)!=0)
		return -1;

	return 0;
*/
	return -1;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<OONet_TcpIp_Manager,ACE_Thread_Mutex>;
template class ACE_Acceptor<OONet_TcpIp_Connection, ACE_SOCK_ACCEPTOR>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<OONet_TcpIp_Manager,ACE_Thread_Mutex>
#pragma instantiate ACE_Acceptor<OONet_TcpIp_Connection, ACE_SOCK_ACCEPTOR>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */