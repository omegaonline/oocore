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

	HRESULT init(const OOObject::uint32_t& key, OOUtil::Object_Ptr<OOObject::TypeInfo>& type_info);

DECLARE_REGISTRY_RESOURCEID(IDR_OODISPATCH)

BEGIN_COM_MAP(COODispatch)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

private:
	OOObject::uint32_t						m_OO_key;
	OOUtil::Object_Ptr<OOObject::TypeInfo>	m_ptr_OO_TypeInfo;

	VARTYPE OOTypeToVARTYPE(OOObject::TypeInfo::Type_t type);
	HRESULT WriteParam(const CComVariant& var, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& ptrOut);
	HRESULT WriteString(const BSTR bstr, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& out);
	HRESULT WriteArray(SAFEARRAY* parray, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& ptrOut);
	HRESULT ReadParam(VARIANT* var, OOObject::TypeInfo::Type_t type, OOUtil::InputStream_Ptr& ptrIn);
	HRESULT ReadString(BSTR& bstr, OOObject::TypeInfo::Type_t type, OOUtil::InputStream_Ptr& in);

	template <class T>
	HRESULT Write(const T& val, OOUtil::OutputStream_Ptr& out)
	{
		return (out.write(val) == 0 ? S_OK : STG_E_ABNORMALAPIEXIT);
	}

	template <class T>
	HRESULT Read(T& val, OOUtil::InputStream_Ptr& in)
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
