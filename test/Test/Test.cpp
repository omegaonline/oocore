#include "./Test.h"

DEFINE_IID(Test::Test,6AAE8C33-699A-4414-AF84-25E74E693207);

BEGIN_PROXY_STUB_MAP(Test)
	PROXY_STUB_AUTO_ENTRY(Test::Test)
END_PROXY_STUB_MAP()

/*static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name);
extern "C" __declspec (dllexport) int RegisterLib(bool bRegister) 
{ 
	return CreateProxyStub((bRegister?2:3),0,OOObject::guid_t::NIL,0,OOObject::cookie_t(),0,0, "Test" ); 
}
extern "C" __declspec (dllexport) int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy) 
{ 
	return CreateProxyStub(0,manager,iid,0,key,proxy,0,0); 
} 
extern "C" __declspec (dllexport) int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOCore::Stub** stub) 
{
	return CreateProxyStub(1,manager,iid,obj,key,0,stub,0); 
}
static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name) 
{
	if ((type==0 && proxy==0) || (type==1 && stub==0) || ((type==2 || type==3) && dll_name==0) || type<0 || type>3) 
	{ 
		(*_errno()) = 22; 
		do 
		{ 
			int __ace_error = ACE_Log_Msg::last_error_adapter (); 
			ACE_Log_Msg *ace___ = ACE_Log_Msg::instance (); 
			ace___->conditional_set ("c:\\work\\omegaonline\\development\\trunk\\test\\test\\test.cpp", 5, -1, __ace_error); 
			ace___->log (LM_ERROR,"(%P|%t) Invalid NULL pointer\n"); 
			return -1; 
		} while (0);
	} 
	if (type==0) *proxy=0; 
	if (type==1) *stub=0;
	if (type==2) 
		OOCore::RegisterProxyStub(Test::Test::IID, dll_name ); 
	else if (type==3) 
		OOCore::UnregisterProxyStub(Test::Test::IID, dll_name ); 
	else if (iid==Test::Test::IID) 
	{ 
		if (type==0) 
			*proxy=Test::Test_Proxy_Stub_Impl::create_proxy(manager,key); 
		else if (type==1) 
			*stub=Test::Test_Proxy_Stub_Impl::create_stub(manager,key,static_cast<Test::Test*>(obj));
		goto end;
	}
end: 
	if ((type==0 && *proxy==0) || (type==1 && *stub==0)) 
	{ 
		(*_errno()) = 2; do { int __ace_error = ACE_Log_Msg::last_error_adapter (); ACE_Log_Msg *ace___ = ACE_Log_Msg::instance (); ace___->conditional_set ("c:\\work\\omegaonline\\development\\trunk\\test\\test\\test.cpp", 7, -1, __ace_error); ace___->log (LM_DEBUG,"(%P|%t) Proxy/Stub create failed\n"); return -1; } while (0); 
	}
	if (type==0) 
		(*proxy)->AddRef(); 
	if (type==1) 
		(*stub)->AddRef(); 
	return 0; 
}*/

DEFINE_CLSID(Test,7A5701A9-28FD-4fa0-8D95-77D00C753444);

#ifdef ACE_WIN32
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		::DisableThreadLibraryCalls(instance);
	}
	
	return TRUE;
}
#endif
