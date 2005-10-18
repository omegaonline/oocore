#pragma once

#include "../OOCore/Client_Service.h"

class OOControlService;

class OOControlClient :
	public OOObj::Object_Impl<OOObj::Client_Service>
{
public:
	virtual int QueryInterface(const OOObj::GUID& iid, Object** ppVal);

	virtual int Stop(OOObj::bool_t force, OOObj::uint16_t* remaining);
	virtual int StopPending(OOObj::bool_t* pending);
	virtual int StayAlive();


	// Test members
	virtual int Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray);
	virtual int Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray);
	virtual int Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray);

	static int Create(OOObj::Client_Service** pObj);

private:
	OOControlClient(OOControlService* service);
	virtual ~OOControlClient(void);

	OOControlService* m_service;
};
