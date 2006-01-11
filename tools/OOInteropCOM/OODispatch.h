// OODispatch.h : Declaration of the COODispatch

#pragma once
#include "resource.h"       // main symbols

#include "OOInteropCOM.h"

// COODispatch
class ATL_NO_VTABLE COODispatch : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IDispatch
{
public:
	COODispatch()
	{
	}

	HRESULT init(const OOCore::ProxyStubManager::cookie_t& key, OOCore::Object_Ptr<OOCore::TypeInfo>& type_info);

DECLARE_REGISTRY_RESOURCEID(IDR_OODISPATCH)

BEGIN_COM_MAP(COODispatch)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

private:
	OOCore::ProxyStubManager::cookie_t		m_OO_key;
	OOCore::Object_Ptr<OOCore::TypeInfo>	m_ptr_OO_TypeInfo;

	VARTYPE OOTypeToVARTYPE(OOCore::TypeInfo::Type_t type);
	HRESULT WriteParam(const CComVariant& var, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::OutputStream>& ptrOut);
	HRESULT WriteString(const BSTR bstr, OOCore::TypeInfo::Type_t type, OOCore::OutputStream_Wrapper& out);
	HRESULT WriteArray(SAFEARRAY* parray, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::OutputStream>& ptrOut);
	HRESULT ReadParam(VARIANT* var, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::InputStream>& ptrIn);
	HRESULT ReadString(BSTR& bstr, OOCore::TypeInfo::Type_t type, OOCore::InputStream_Wrapper& in);

	template <class T>
	HRESULT Write(const T& val, OOCore::OutputStream_Wrapper& out)
	{
		return (out.write(val) == 0 ? S_OK : STG_E_ABNORMALAPIEXIT);
	}

	template <class T>
	HRESULT Read(T& val, OOCore::InputStream_Wrapper& in)
	{
		return (in.read(val) == 0 ? S_OK : STG_E_ABNORMALAPIEXIT);
	}
	
// IDispatch members
public:
	STDMETHOD(GetTypeInfoCount)(/* [out] */ UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(/* [in] */ UINT iTInfo, /* [in] */ LCID lcid, /* [out] */ ITypeInfo **ppTInfo);
	STDMETHOD(GetIDsOfNames)(/* [in] */ REFIID riid, /* [size_is][in] */ LPOLESTR *rgszNames, /* [in] */ UINT cNames, /* [in] */ LCID lcid, /* [size_is][out] */ DISPID *rgDispId);
	STDMETHOD(Invoke)(/* [in] */ DISPID dispIdMember, /* [in] */ REFIID riid, /* [in] */ LCID lcid, /* [in] */ WORD wFlags, /* [out][in] */ DISPPARAMS *pDispParams, /* [out] */ VARIANT *pVarResult, /* [out] */ EXCEPINFO *pExcepInfo, /* [out] */ UINT *puArgErr);
};

OBJECT_ENTRY_NON_CREATEABLE_EX_AUTO(GUID_NULL,COODispatch)
