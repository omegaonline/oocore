#include "./Svc_Manager.h"

#include <ace/Service_Config.h>
#include <ace/Get_Opt.h>

#include "./OOps_export.h"

// Declare the service
ACE_FACTORY_DEFINE(OOps,OOps_Svc_Manager)

OOps_Svc_Manager::OOps_Svc_Manager(void) :
	ACE_Task<ACE_MT_SYNCH>(),
	m_Local_Service("OOps"),
	m_Remote_Service("OOps_Remote")
{
}

OOps_Svc_Manager::~OOps_Svc_Manager(void)
{
}

int OOps_Svc_Manager::init(int argc, ACE_TCHAR *argv[])
{
	if (ACE::debug())
		ACE_DEBUG((LM_DEBUG,ACE_TEXT("Svc_Manager::init\n")));

	// Parse cmd line first
	ACE_Get_Opt cmd_opts(argc,argv,ACE_TEXT(":"),0);
	int option;
	u_short uPort = 0;
	while ((option = cmd_opts()) != EOF)
	{
		switch (option)
		{
		case ACE_TEXT(':'):
			ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Missing argument for -%c.\n"),cmd_opts.opt_opt()),-1);
			break;

		default:
			break;
		}
	}

	m_Local_Service.open(true);
	m_Remote_Service.open(false);
	
	return 0;
}

void OOps_Svc_Manager::handle_shutdown()
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OOps_Svc_Manager::shutdown\n")));

	m_Local_Service.close();
	m_Remote_Service.close();
}

int OOps_Svc_Manager::fini(void)
{
	ACE_DEBUG((LM_DEBUG,ACE_TEXT("OOps_Svc_Manager::fini\n")));

	return 0;
}

#if defined (ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION)
template class ACE_DLL_Singleton_T<OOps_Svc_Manager,ACE_Thread_Mutex>;
#elif defined (ACE_HAS_TEMPLATE_INSTANTIATION_PRAGMA)
#pragma instantiate ACE_DLL_Singleton_T<OOps_Svc_Manager,ACE_Thread_Mutex>
#endif /* ACE_HAS_EXPLICIT_TEMPLATE_INSTANTIATION */