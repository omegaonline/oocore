// OOInteropCOM.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"
#include "OOInteropCOM.h"
#include "COMClassFactory.h"

class COOInteropCOMModule : public CAtlDllModuleT< COOInteropCOMModule >
{
public:
	COOInteropCOMModule() :
	  m_bInitCalled(0)
	{ }

	LONG m_bInitCalled;

	//DECLARE_LIBID(LIBID_OOInteropCOMLib)

	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OOINTEROPCOM, "{3974F4CB-8571-4427-8155-DCEA55416579}")
};

COOInteropCOMModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_THREAD_ATTACH)
		// Open Winsock (no-op on other platforms).
		ACE_OS::socket_init(ACE_WSOCK_VERSION);

    hInstance;
    return _AtlModule.DllMain(dwReason, lpReserved);
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return _AtlModule.DllCanUnloadNow();
}

static void __stdcall Shutdown(DWORD_PTR dw)
{
	if (InterlockedExchange(&_AtlModule.m_bInitCalled,0)==1)
	{
		OOObject::Term();
		ACE::fini();
	}
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	if (InterlockedExchange(&_AtlModule.m_bInitCalled,1)==0)
	{
		ACE::init();
		OOObject::Init();

		if (INTEROP::instance()->Open() != 0)
			return E_UNEXPECTED;

		_AtlModule.AddTermFunc(Shutdown,0);
	}

    HRESULT hr = _AtlModule.DllGetClassObject(rclsid, riid, ppv);
	if (hr != S_OK)
		hr = CCOMClassFactory::GetClassObject(rclsid, riid, ppv);
	
	return hr;
}

// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    HRESULT hr = _AtlModule.DllRegisterServer(FALSE);
	return hr;
}

// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
	HRESULT hr = _AtlModule.DllUnregisterServer(FALSE);
	return hr;
}
