#include "./OOControlClient.h"
#include "./OOControlService.h"

int OOControlClient::Create(OOObj::Client_Service** pObj)
{
	ACE_NEW_RETURN(*pObj,OOControlClient(OOCONTROL_SERVICE::instance()),-1);
	
	(*pObj)->AddRef();
	return 0;
}

OOControlClient::OOControlClient(OOControlService* service) :
	m_service(service)
{
	m_service->add_client(this);
}

OOControlClient::~OOControlClient(void)
{
	m_service->remove_client(this);
}

int OOControlClient::QueryInterface(const OOObj::GUID& iid, OOObj::Object** ppVal)
{
	if (iid == OOObj::Object::IID ||
		iid == OOObj::Client_Service::IID)
	{
		*ppVal = this;
		AddRef();
		return 0;
	}

	ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) QI for unsupported interface\n")),-1);
}

int OOControlClient::Stop(OOObj::bool_t force, OOObj::uint16_t* remaining)
{
	if (m_service==0)
		return -1;

	*remaining = m_service->stop(force);
	return 0;
}

int OOControlClient::StopPending(OOObj::bool_t* pending)
{
	if (m_service==0)
		return -1;

	*pending = m_service->stop_pending();
	return 0;
}

int OOControlClient::StayAlive()
{
	if (m_service==0)
		return -1;

	m_service->stay_alive();
	return 0;
}

int OOControlClient::Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray)
{
	return 0;
}

int OOControlClient::Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray)
{
	*count = 14;
	*pArray = static_cast<OOObj::uint16_t*>(OOCore_Alloc(*count * sizeof(OOObj::uint16_t)));
	
	for (OOObj::uint32_t i=0;i<*count;++i)
		(*pArray)[i] = (*count - i);

	return 0;
}

int OOControlClient::Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray)
{
	OOCore_Free(*pArray);

	*count = 16;
	*pArray = static_cast<OOObj::uint16_t*>(OOCore_Alloc(*count * sizeof(OOObj::uint16_t)));
	
	for (OOObj::uint32_t i=0;i<*count;++i)
		(*pArray)[i] = (*count - i);

	return 0;
}
