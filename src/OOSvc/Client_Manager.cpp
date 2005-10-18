#include "./Client_Manager.h"

#include <ace/Service_Config.h>
#include <ace/Get_Opt.h>

// Declare the service
ACE_FACTORY_DEFINE(OOSvc,OOSvc_Client_Manager)

OOSvc_Client_Manager::OOSvc_Client_Manager(void) :
	ACE_Task<ACE_MT_SYNCH>()
{
}

OOSvc_Client_Manager::~OOSvc_Client_Manager(void)
{
}

int OOSvc_Client_Manager::init(int argc, ACE_TCHAR *argv[])
{
	if (ACE::debug())
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("Client_Manager::init\n")));

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

	// Open the core binding
	int ret = BINDING::instance()->launch(true);
	if (ret == -1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("OOCore_Binding::open failed.\n")),-1);
	else if (ret==1)
	{
		// Already running on this machine... Fail
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Client_Manager already running.\n")),-1);
	}
	
	ACE_MEM_Addr port_addr(uPort);
	if (m_acceptor.open(port_addr) == -1)
	{
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("%p\n"),ACE_TEXT("Accept failed")),-1);
	}

	if (m_acceptor.acceptor().get_local_addr(port_addr)==-1)
		return -1;
	
	uPort = port_addr.get_port_number();

	ACE_TCHAR szBuf[24];
	ACE_OS::sprintf(szBuf,ACE_TEXT("%u"),uPort);
	if (BINDING::instance()->rebind(ACE_TEXT("local_port"),szBuf)==-1)
		return -1;
	
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("Listening on port %u for client connections.\n"),uPort));
		
	return 0;
}

void OOSvc_Client_Manager::handle_shutdown()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OOSvc_Client_Manager::shutdown\n")));

	m_acceptor.close();
}

int OOSvc_Client_Manager::fini(void)
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OOSvc_Client_Manager::fini\n")));

	return 0;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<OOSvc_Client_Manager,ACE_Thread_Mutex>;
template class ACE_Acceptor<OOSvc_Client_Connection, ACE_MEM_ACCEPTOR>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<OOSvc_Client_Manager,ACE_Thread_Mutex>
#pragma instantiate ACE_Acceptor<OOSvc_Client_Connection, ACE_MEM_ACCEPTOR>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */