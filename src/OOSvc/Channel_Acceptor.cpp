#include "./Channel_Acceptor.h"

#include "../OOCore/Channel.h"

#include "./Service_Manager.h"

OOSvc_Channel_Acceptor_Base::OOSvc_Channel_Acceptor_Base(const char* name) :
	m_name(name)
{
}

OOSvc_Channel_Acceptor_Base::~OOSvc_Channel_Acceptor_Base()
{
	if (m_name)
		close();
}

int OOSvc_Channel_Acceptor_Base::open(bool bLocal)
{
	return SERVICE_MANAGER::instance()->register_service(this,bLocal);
}

int OOSvc_Channel_Acceptor_Base::close()
{
	if (SERVICE_MANAGER::instance()->unregister_service(m_name) == -1)
		return -1;
	
	m_name = 0;

	return 0;
}

const char* OOSvc_Channel_Acceptor_Base::name()
{
	return m_name;
}
