#include "StdAfx.h"
#include ".\comclassfactory.h"
#include ".\oodispatch.h"

CCOMClassFactory::CCOMClassFactory(void)
{
}

CCOMClassFactory::~CCOMClassFactory(void)
{
}

HRESULT CCOMClassFactory::init(REFCLSID rclsid)
{
	// Find the CLSID entry in the registry
	LPOLESTR pszGuid = NULL;
	HRESULT hr = StringFromCLSID(rclsid,&pszGuid);
	if (FAILED(hr))
		return hr;

	CString strGuid(pszGuid);
	CoTaskMemFree(pszGuid);

	CRegKey key;
	LONG lRes = 0;
	if ((lRes=key.Open(HKEY_LOCAL_MACHINE,_T("SOFTWARE\\Classes\\CLSID\\")+strGuid)) != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);

	// Get the default iid
	GUID com_iid;
	if ((lRes=key.QueryGUIDValue(_T("OO_Default_IID"),com_iid)) != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(lRes);
	
	// Translate the iid
	m_OO_iid.Data1 = com_iid.Data1;
	m_OO_iid.Data2 = com_iid.Data2;
	m_OO_iid.Data3 = com_iid.Data3;
	memcpy(&m_OO_iid.Data4,&com_iid.Data4,8);

	// Translate and stash the guid
	m_OO_clsid.Data1 = rclsid.Data1;
	m_OO_clsid.Data2 = rclsid.Data2;
	m_OO_clsid.Data3 = rclsid.Data3;
	memcpy(&m_OO_clsid.Data4,&rclsid.Data4,8);

	// Get the type info for iid
	if (OOCore::GetTypeInfo(m_OO_iid,&m_ptr_OO_TypeInfo) != 0)
		return CLASS_E_CLASSNOTAVAILABLE;

    // DO SOME TESTS...
	/*size_t method_count;
	const OOObject::char_t* type_name;
	m_ptr_OO_TypeInfo->GetMetaInfo(&type_name,&method_count);
	
	for (size_t i=0;i<method_count;++i)
	{
		const OOObject::char_t* method_name;
		size_t param_count;
		OOCore::TypeInfo::Method_Attributes_t attr;
		OOObject::uint16_t wait_secs;
        m_ptr_OO_TypeInfo->GetMethodInfo(i,&method_name,&param_count,&attr,&wait_secs);

		ACE_DEBUG((LM_DEBUG,ACE_TEXT("%s::%s("),type_name,method_name));

		for (size_t j=0;j<param_count;++j)
		{
			const OOObject::char_t* param_name;
			OOCore::TypeInfo::Type_t type;
			m_ptr_OO_TypeInfo->GetParamInfo(i,j,&param_name,&type);

			if (j>0)
				ACE_DEBUG((LM_DEBUG,ACE_TEXT(", ")));

			ACE_DEBUG((LM_DEBUG,ACE_TEXT("0x%X %s"),type,param_name));
		}

		ACE_DEBUG((LM_DEBUG,ACE_TEXT(")\n")));
	}*/

	return S_OK;
}

STDMETHODIMP CCOMClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj)
{
	// First create the OO object
	OOCore::Object_Ptr<OOObject::Object> ptrObj;
	OOCore::ProxyStubManager::cookie_t key;
	if (INTEROP::instance()->CreateObject(m_OO_clsid,m_OO_iid,&key,&ptrObj) != 0)
		return CLASS_E_CLASSNOTAVAILABLE;

	// Now create the OODispatch object
	CComPolyObject<COODispatch>* oodisp = NULL;
	HRESULT hr = CComPolyObject<COODispatch>::CreateInstance(pUnkOuter,&oodisp);
	if (FAILED(hr))
	{
		delete oodisp;
		return hr;
	}

	// Init the dispatch object
	hr = oodisp->m_contained.init(key,m_ptr_OO_TypeInfo);
	if (FAILED(hr))
	{
		delete oodisp;
		return hr;
	}

	// Return via QI()
	hr = oodisp->QueryInterface(riid,ppvObj);
	if (FAILED(hr))
		delete oodisp;

	return hr;
}

HRESULT CCOMClassFactory::GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	// Create a CCOMClassFactory
	CComObject<CCOMClassFactory>* obj = NULL;
	HRESULT hr = CComObject<CCOMClassFactory>::CreateInstance(&obj);
	if (FAILED(hr))
		return hr;
	
	// Check we can create objects of class clsid
	hr = obj->init(rclsid);
	if (FAILED(hr))
	{
		delete obj;
		return hr;
	}
	
	// return via QI()
	hr = obj->QueryInterface(riid,ppv);
	if (FAILED(hr))
		delete obj;
	
	return hr;
}
