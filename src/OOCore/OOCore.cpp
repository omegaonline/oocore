#include "./OOCore.h"

#include <ace/OS_NS_wchar.h>

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
DEFINE_IID(OOCore::Test,6AAE8C33-699A-4414-AF84-25E74E693207);

DEFINE_CLSID(OOCore::CLSID_Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

#endif

namespace OOCore
{
namespace Impl
{
	OOCore_Export int RegisterAsServer();
	OOCore_Export int SetServerPort(OOObject::uint16_t uPort);

	bool g_IsServer = false;

#ifdef ACE_WIN32
	HINSTANCE g_hInstance = NULL;
#endif
};
};

#ifdef ACE_WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		ACE::init();

		OOCore::Impl::g_hInstance = instance;
		::DisableThreadLibraryCalls(instance);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		ACE::fini();
	}
	return TRUE;
}
#endif

OOCore_Export int 
OOCore::Impl::RegisterAsServer()
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
OOCore::Impl::SetServerPort(OOObject::uint16_t uPort)
{
	ACE_TCHAR szBuf[24];
	ACE_OS::sprintf(szBuf,ACE_TEXT("%u"),uPort);
	if (Impl::BINDING::instance()->rebind(ACE_TEXT("local_port"),szBuf)==-1)
		return -1;

	return 0;
}

OOCore_Export int 
OOCore::RegisterProxyStub(const OOObject::guid_t& iid, const char* dll_name)
{
	ACE_TString value(Impl::guid_to_string(iid));

	return Impl::BINDING::instance()->rebind(value.c_str(),ACE_TEXT_CHAR_TO_TCHAR(dll_name));
}

OOCore_Export int 
OOCore::UnregisterProxyStub(const OOObject::guid_t& iid, const char* dll_name)
{
	ACE_TString value(Impl::guid_to_string(iid));

	// Find the stub DLL name
	ACE_TString name;
	if (Impl::BINDING::instance()->find(value.c_str(),name) != 0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) No proxy/stub registered\n")),-1);
		
	// Check if it's us
	if (!(name==ACE_TEXT_CHAR_TO_TCHAR(dll_name)))
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Proxy/stub not registered to this library\n")),-1);

	return Impl::BINDING::instance()->unbind(value.c_str());
}

OOCore_Export OOObject::int32_t  
OOCore::AddObjectFactory(const OOObject::guid_t& clsid, ObjectFactory* pFactory)
{
	if (Impl::g_IsServer)
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
	if (Impl::g_IsServer)
	{
		return Impl::OBJECT_FACTORY::instance()->remove_object_factory(clsid);
	}
	else
	{
		return Impl::CONNECTION_MANAGER::instance()->RemoveObjectFactory(clsid);
	}
}
