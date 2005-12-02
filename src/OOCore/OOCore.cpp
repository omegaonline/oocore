#include "./OOCore.h"
#include "./Register.h"
#include "./Binding.h"
#include "./Connection_Manager.h"
#include "./Object_Factory.h"

DEFINE_IID(OOObject::Object,45F040A3-5386-413e-AB21-7FA35EFCB7DD);
DEFINE_IID(OOCore::Stub,D8B1513D-967B-429e-8403-31650213DA21);
DEFINE_IID(OOCore::InputStream,86F468DB-953F-4be0-A8EB-D9A344C104E3);
DEFINE_IID(OOCore::OutputStream,0FA60065-8C8A-463b-9B01-D080E03EF39F);
DEFINE_IID(OOCore::Transport,33EE56E9-9748-43ce-A71C-516ACE28925C);
DEFINE_IID(OOCore::ProxyStubManager,F3EB63E5-602A-4155-8F52-F11FF502EFE5);
DEFINE_IID(OOCore::ObjectFactory,E2760ABA-1BAA-4c7b-89B2-466320296D1D);

#ifdef _DEBUG
#include "./Test.h"

DEFINE_IID(OOCore::Server,B4B5BF71-58DF-4001-BD0B-72496463E3C3);
REGISTER_PROXYSTUB(OOCore,Server,OOCore)

DEFINE_IID(OOCore::Test,6AAE8C33-699A-4414-AF84-25E74E693207);
REGISTER_PROXYSTUB(OOCore,Test,OOCore)

DEFINE_CLSID(OOCore::CLSID_Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

#endif

bool g_IsServer = false;

OOCore_Export int 
OOCore::InitAsServer()
{
	if (Impl::BINDING::instance()->launch(true) == 0)
	{
		// This is the only place this is set!
		g_IsServer = true;
		return 0;
	}
	return -1;
}

OOCore_Export int
OOCore::SetServerPort(OOObject::uint16_t uPort)
{
	ACE_TCHAR szBuf[24];
	ACE_OS::sprintf(szBuf,ACE_TEXT("%u"),uPort);
	if (Impl::BINDING::instance()->rebind(ACE_TEXT("local_port"),szBuf)==-1)
		return -1;

	return 0;
}

OOCore_Export OOObject::int32_t  
OOCore::AddObjectFactory(const OOObject::guid_t& clsid, ObjectFactory* pFactory)
{
	if (g_IsServer)
	{
		return Impl::OBJECT_FACTORY::instance()->add_object_factory(clsid,pFactory);
	}
	else
	{
		return Impl::CONNECTION_MANAGER::instance()->AddObjectFactory(clsid,pFactory);
	}
}

OOCore_Export OOObject::int32_t  
OOCore::RemoveObjectFactory(const OOObject::guid_t& clsid)
{
	if (g_IsServer)
	{
		return Impl::OBJECT_FACTORY::instance()->remove_object_factory(clsid);
	}
	else
	{
		return Impl::CONNECTION_MANAGER::instance()->RemoveObjectFactory(clsid);
	}
}
