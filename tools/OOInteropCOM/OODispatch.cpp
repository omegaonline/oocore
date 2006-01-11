// OODispatch.cpp : Implementation of COODispatch

#include "stdafx.h"
#include "OODispatch.h"
#include ".\oodispatch.h"

// COODispatch
HRESULT COODispatch::init(const OOCore::ProxyStubManager::cookie_t& key, OOCore::Object_Ptr<OOCore::TypeInfo>& type_info)
{
	m_OO_key = key;
	m_ptr_OO_TypeInfo = type_info;

	return S_OK;
}

HRESULT COODispatch::ReadParam(VARIANT* var, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::InputStream>& ptrIn)
{
	/*VT_ARRAY	= 0x2000*/
	
	switch (var->vt)
	{
	case VT_DISPATCH:
	case VT_UNKNOWN:

	case VT_I8:
		return Read(var->llVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_I4:
		{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Read(v,OOCore::InputStream_Wrapper(ptrIn))) == S_OK)
				var->lVal = v;
			return hr;
		}

    case VT_UI1:
		return Read(var->bVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_I2:
		return Read(var->iVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_R4:
		return Read(var->fltVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_R8:
		return Read(var->dblVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_BOOL:
		return Read(var->boolVal,OOCore::InputStream_Wrapper(ptrIn));
    
	case (VT_BYREF | VT_UI1):
		return Read(*var->pbVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_I2):
		return Read(*var->piVal,OOCore::InputStream_Wrapper(ptrIn));
    
	case (VT_BYREF | VT_I4):
		{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Read(v,OOCore::InputStream_Wrapper(ptrIn))) == S_OK)
				*var->plVal = v;
			return hr;
		}

	case (VT_BYREF | VT_I8):
		return Read(*var->pllVal,OOCore::InputStream_Wrapper(ptrIn));
    
	case (VT_BYREF | VT_R4):
		return Read(*var->pfltVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_R8):
		return Read(*var->pdblVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_BOOL):
		return Read(*var->pboolVal,OOCore::InputStream_Wrapper(ptrIn));
    
    case VT_BSTR:
		if (type & OOCore::TypeInfo::string)
			return ReadString(var->bstrVal,type,OOCore::InputStream_Wrapper(ptrIn));
		else
			return DISP_E_BADVARTYPE;		

	case (VT_BYREF | VT_BSTR):
		if (type & OOCore::TypeInfo::string)
			return ReadString(*var->pbstrVal,type,OOCore::InputStream_Wrapper(ptrIn));
		else
			return DISP_E_BADVARTYPE;		
    /*
	case IUnknown*: punkVal;
    case IDispatch*: pdispVal;
    
    IUnknown **ppunkVal;
    IDispatch **ppdispVal;

	case SAFEARRAY*: parray;
    SAFEARRAY **pparray;
    */

	case (VT_BYREF | VT_VARIANT): 
		return ReadParam(var->pvarVal,type,ptrIn);
    
    case VT_I1:
		return Read(var->cVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_UI2:
		return Read(var->uiVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_UI4:
		{
			// This is bust for some reason!
			OOObject::uint32_t v;
			HRESULT hr;
			if ((hr=Read(v,OOCore::InputStream_Wrapper(ptrIn))) == S_OK)
				var->ulVal = v;
			return hr;
		}

    case VT_UI8:
		return Read(var->ullVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_INT:
		return Read(var->intVal,OOCore::InputStream_Wrapper(ptrIn));

    case VT_UINT:
		return Read(var->uintVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_I1):
		return Read(*var->pcVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_UI2):
		return Read(*var->puiVal,OOCore::InputStream_Wrapper(ptrIn));

	case (VT_BYREF | VT_UI4):
		{
			// This is bust for some reason!
			OOObject::uint32_t v;
			HRESULT hr;
			if ((hr=Read(v,OOCore::InputStream_Wrapper(ptrIn))) == S_OK)
				*var->pulVal = v;
			return hr;
		}

    case (VT_BYREF | VT_UI8):
		return Read(*var->pullVal,OOCore::InputStream_Wrapper(ptrIn));

    case (VT_BYREF | VT_INT):
		return Read(*var->pintVal,OOCore::InputStream_Wrapper(ptrIn));

    case (VT_BYREF | VT_UINT):
		return Read(*var->puintVal,OOCore::InputStream_Wrapper(ptrIn));
    	
	// We can't handle anything else!
	default:
		return DISP_E_BADVARTYPE;
	}
}

HRESULT COODispatch::ReadString(BSTR& bstr, OOCore::TypeInfo::Type_t type, OOCore::InputStream_Wrapper& in)
{
	::SysFreeString(bstr);
	bstr=NULL;

	OOObject::uint32_t nLen = 0;
	HRESULT hr = Read(nLen,in);
	if FAILED(hr)
		return hr;

	if (nLen==0)
		return S_OK;

	if (type & OOCore::TypeInfo::char_t)
	{
		LPSTR pszText = new char[nLen+1];
		for (UINT i=0;i<nLen;++i)
		{
			hr = Read(pszText[i],in);
			if FAILED(hr)
			{
				delete [] pszText;
				return hr;
			}
		}
		pszText[nLen] = 0;

		bstr = ::SysAllocStringLen(CA2W(pszText),nLen);

		delete [] pszText;

		return S_OK;
	}
	else if (type & OOCore::TypeInfo::int16_t)
	{
		LPWSTR pszText = new wchar_t[nLen+1];
		for (UINT i=0;i<nLen;++i)
		{
			OOObject::int16_t ch;
			hr = Read(ch,in);
			if FAILED(hr)
			{
				delete [] pszText;
				return hr;
			}

			pszText[i] = ch;
		}
		pszText[nLen] = 0;

		bstr = ::SysAllocStringLen(pszText,nLen);

		delete [] pszText;

		return S_OK;
	}

	return DISP_E_BADVARTYPE;
}

HRESULT COODispatch::WriteParam(const CComVariant& var, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::OutputStream>& ptrOut)
{
	if (var.vt & (VT_ARRAY | VT_BYREF))
		return WriteArray(*var.pparray,type,ptrOut);
	else if (var.vt & VT_ARRAY)
		return WriteArray(var.parray,type,ptrOut);
	
	switch (var.vt)
	{
	case VT_DISPATCH:
	case VT_UNKNOWN:

	case VT_I8:
		return Write(var.llVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_I4:
		return Write(var.lVal,OOCore::OutputStream_Wrapper(ptrOut));
		/*{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Write(v,OOCore::OutputStream_Wrapper(ptrOut))) == S_OK)
				var->lVal = v;
			return hr;
		}*/

    case VT_UI1:
		return Write(var.bVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_I2:
		return Write(var.iVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_R4:
		return Write(var.fltVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_R8:
		return Write(var.dblVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_BOOL:
		return Write(var.boolVal,OOCore::OutputStream_Wrapper(ptrOut));
    
	case (VT_BYREF | VT_UI1):
		return Write(*var.pbVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_I2):
		return Write(*var.piVal,OOCore::OutputStream_Wrapper(ptrOut));
    
	case (VT_BYREF | VT_I4):
		return Write(*var.plVal,OOCore::OutputStream_Wrapper(ptrOut));
		
	case (VT_BYREF | VT_I8):
		return Write(*var.pllVal,OOCore::OutputStream_Wrapper(ptrOut));
    
	case (VT_BYREF | VT_R4):
		return Write(*var.pfltVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_R8):
		return Write(*var.pdblVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_BOOL):
		return Write(*var.pboolVal,OOCore::OutputStream_Wrapper(ptrOut));
    
    case VT_BSTR:
		if (type & OOCore::TypeInfo::string)
			return WriteString(var.bstrVal,type,OOCore::OutputStream_Wrapper(ptrOut));
		else
			return DISP_E_BADVARTYPE;		

	case (VT_BYREF | VT_BSTR):
		if (type & OOCore::TypeInfo::string)
			return WriteString(*var.pbstrVal,type,OOCore::OutputStream_Wrapper(ptrOut));
		else
			return DISP_E_BADVARTYPE;		
    /*
	case IUnknown*: punkVal;
    case IDispatch*: pdispVal;
    
    IUnknown **ppunkVal;
    IDispatch **ppdispVal;
    */

	case (VT_BYREF | VT_VARIANT): 
		return WriteParam(*var.pvarVal,type,ptrOut);
    
    case VT_I1:
		return Write(var.cVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_UI2:
		return Write(var.uiVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_UI4:
		return Write(static_cast<OOObject::uint32_t>(var.ulVal),OOCore::OutputStream_Wrapper(ptrOut));

    case VT_UI8:
		return Write(var.ullVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_INT:
		return Write(var.intVal,OOCore::OutputStream_Wrapper(ptrOut));

    case VT_UINT:
		return Write(var.uintVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_I1):
		return Write(*var.pcVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_UI2):
		return Write(*var.puiVal,OOCore::OutputStream_Wrapper(ptrOut));

	case (VT_BYREF | VT_UI4):
		return Write(static_cast<OOObject::int32_t>(*var.pulVal),OOCore::OutputStream_Wrapper(ptrOut));

    case (VT_BYREF | VT_UI8):
		return Write(*var.pullVal,OOCore::OutputStream_Wrapper(ptrOut));

    case (VT_BYREF | VT_INT):
		return Write(*var.pintVal,OOCore::OutputStream_Wrapper(ptrOut));

    case (VT_BYREF | VT_UINT):
		return Write(*var.puintVal,OOCore::OutputStream_Wrapper(ptrOut));
    	
	// We can't handle anything else!
	default:
		return DISP_E_BADVARTYPE;
	}
}

HRESULT COODispatch::WriteArray(SAFEARRAY* parray, OOCore::TypeInfo::Type_t type, OOCore::Object_Ptr<OOCore::OutputStream>& ptrOut)
{
	long ubound,lbound;

	// Remove the array tag from type
	type &= ~OOCore::TypeInfo::array;

	VARTYPE vt = OOTypeToVARTYPE(type);

	if (SafeArrayGetDim(parray) != 1)
		return DISP_E_BADVARTYPE;

	HRESULT hr = SafeArrayGetUBound(parray,1,&ubound);
	if FAILED(hr)
		return hr;

	hr = SafeArrayGetLBound(parray,1,&lbound);
	if FAILED(hr)
		return hr;

	SafeArrayLock(parray);

	for (long i=lbound;i<=ubound;++i)
	{
		CComVariant elem;
		hr = SafeArrayGetElement(parray,&i,&elem);
		if FAILED(hr)
			break;

		hr = elem.ChangeType(vt);
		if FAILED(hr)
			break;

		hr = WriteParam(elem,type,ptrOut);
		if FAILED(hr)
			break;
	}

	SafeArrayUnlock(parray);

	return hr;
}

HRESULT COODispatch::WriteString(const BSTR bstr, OOCore::TypeInfo::Type_t type, OOCore::OutputStream_Wrapper& out)
{
	OOObject::uint32_t nLen = ::SysStringLen(bstr);
	HRESULT hr = Write(nLen,out);
	if FAILED(hr)
		return hr;

	if (type & OOCore::TypeInfo::char_t)
	{
		LPSTR pszText = CW2A(bstr);
		for (UINT i=0;i<nLen;++i)
		{
			hr = Write(pszText[i],out);
			if FAILED(hr)
				return hr;
		}
		return S_OK;
	}
	else if (type & OOCore::TypeInfo::int16_t)
	{
		for (UINT i=0;i<nLen;++i)
		{
			hr = Write(bstr[i],out);
			if FAILED(hr)
				return hr;
		}
		return S_OK;
	}

	return DISP_E_BADVARTYPE;
}

VARTYPE COODispatch::OOTypeToVARTYPE(OOCore::TypeInfo::Type_t type)
{
	// Work out the basic type
	VARTYPE result = VT_EMPTY;
	switch (type & OOCore::TypeInfo::TYPE_MASK)
	{
	case OOCore::TypeInfo::bool_t:
		result = VT_BOOL;
		break;

	case OOCore::TypeInfo::char_t:
		result = VT_I1;
		break;

	case OOCore::TypeInfo::byte_t:
		result = VT_UI1;
		break;

	case OOCore::TypeInfo::int16_t:
		result = VT_I2;
		break;

	case OOCore::TypeInfo::uint16_t:
		result = VT_UI2;
		break;

	case OOCore::TypeInfo::int32_t:
		result = VT_I4;
		break;

	case OOCore::TypeInfo::uint32_t:
		result = VT_UI4;
		break;

	case OOCore::TypeInfo::int64_t:
		result = VT_I8;
		break;

	case OOCore::TypeInfo::uint64_t:
		result = VT_UI8;
		break;

	case OOCore::TypeInfo::real4_t:
		result = VT_R4;
		break;

	case OOCore::TypeInfo::real8_t:
		result = VT_R8;
		break;

	case OOCore::TypeInfo::Object:	
		result = VT_DISPATCH;
		break;

	// The other values make no sense in IDispatch world
	default:
		break;
	}

	OOCore::TypeInfo::Type_t attr_type = (type & OOCore::TypeInfo::ATTR_MASK);

	// Check for possible string types...
	if (attr_type & OOCore::TypeInfo::string)
	{
		if (result == VT_I1 || result==VT_I2)
			result = VT_BSTR;
		else
		{
			// Implement as a simple array
			result |= VT_ARRAY;
		}
	}

	// Check the in and out attributes for pointer values...
	if (attr_type & OOCore::TypeInfo::out)
	{
		// Must be a pointer
		result |= VT_BYREF;
	}

	if (attr_type & OOCore::TypeInfo::array)
	{
		// Must be an array
		result |= VT_ARRAY;
	}

	return result;
}

STDMETHODIMP COODispatch::GetTypeInfoCount(/* [out] */ UINT *pctinfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP COODispatch::GetTypeInfo(/* [in] */ UINT iTInfo, /* [in] */ LCID lcid, /* [out] */ ITypeInfo **ppTInfo)
{
	return E_NOTIMPL;
}

STDMETHODIMP COODispatch::GetIDsOfNames(/* [in] */ REFIID riid, /* [size_is][in] */ LPOLESTR *rgszNames, /* [in] */ UINT cNames, /* [in] */ LCID lcid, /* [size_is][out] */ DISPID *rgDispId)
{
	// Check params
	if (cNames==0)
		return E_UNEXPECTED;

	// Preset results
	for (UINT j=0;j<cNames;++j)
		rgDispId[j] = DISPID_UNKNOWN;

	// Get the meta info
	size_t method_count;
	const OOObject::char_t* type_name;
	if (m_ptr_OO_TypeInfo->GetMetaInfo(&type_name,&method_count) != 0)
		return E_UNEXPECTED;
	
	// Fidn the method matching rgszNames[0]
	size_t param_count = 0;
	for (size_t i=0;i<method_count;++i)
	{
		const OOObject::char_t* method_name;
		OOCore::TypeInfo::Method_Attributes_t attribs;
		OOObject::uint16_t wait_secs;
		if (m_ptr_OO_TypeInfo->GetMethodInfo(i,&method_name,&param_count,&attribs,&wait_secs) != 0)
			return E_UNEXPECTED;

		if (ACE_OS::strcasecmp(method_name,CW2A(rgszNames[0]))==0)
		{
			rgDispId[0] = static_cast<DISPID>(i);
			break;
		}
	}

	// Check we have found it
	// Don't allow access to AddRef(), Release() and QI()
	if (rgDispId[0] == DISPID_UNKNOWN || rgDispId[0] < 3)
		return DISP_E_UNKNOWNNAME;

	// Get the name of every parameter
	for (i=0;i<param_count;++i)
	{
		const OOObject::char_t* param_name;
		OOCore::TypeInfo::Type_t type;
		if (m_ptr_OO_TypeInfo->GetParamInfo(rgDispId[0],i,&param_name,&type) != 0)
			return E_UNEXPECTED;
		
		// Comapre to each member of rgsNames
		for (j=1;j<cNames;++j)
		{
     		if (ACE_OS::strcasecmp(param_name,CW2A(rgszNames[j]))==0)
			{
				rgDispId[j] = static_cast<DISPID>(i);
				break;
			}
		}
	}
	
	// Check to see if we haven't found any
	for (j=0;j<cNames;++j)
		if (rgDispId[j] == DISPID_UNKNOWN)
			return DISP_E_UNKNOWNNAME;

	return S_OK;
}

STDMETHODIMP COODispatch::Invoke(/* [in] */ DISPID dispIdMember, /* [in] */ REFIID riid, /* [in] */ LCID lcid, /* [in] */ WORD wFlags, /* [out][in] */ DISPPARAMS *pDispParams, /* [out] */ VARIANT *pVarResult, /* [out] */ EXCEPINFO *pExcepInfo, /* [out] */ UINT *puArgErr)
{
	if (!::IsEqualIID(riid,IID_NULL))
		return DISP_E_UNKNOWNINTERFACE;

	if (wFlags != DISPATCH_METHOD)
		return DISP_E_MEMBERNOTFOUND;

	// Get the method info
	const OOObject::char_t* name;
	OOCore::TypeInfo::Method_Attributes_t attr;
	size_t param_count;
	OOObject::uint16_t wait_secs;
	if (m_ptr_OO_TypeInfo->GetMethodInfo(dispIdMember,&name,&param_count,&attr,&wait_secs) != 0)
		return DISP_E_MEMBERNOTFOUND;

	// Enumerate params
	HRESULT hr;
	std::vector<std::pair<CComVariant,OOCore::TypeInfo::Type_t> > vecParams;
	for (size_t p=0;p<param_count;++p)
	{
		// Get the param type info
		OOCore::TypeInfo::Type_t type;
		if (m_ptr_OO_TypeInfo->GetParamInfo(dispIdMember,p,&name,&type) != 0)
			return E_UNEXPECTED;

		// Coerce the incoming param to the correct type
		CComVariant param;
		VARTYPE vt = OOTypeToVARTYPE(type);
		UINT arg = 0;
        hr = DispGetParam(pDispParams,p,vt,&param,&arg);
		if (hr==DISP_E_TYPEMISMATCH)
		{
			// We can work around ByRef -> ByVal conversions
			if ((pDispParams->rgvarg[arg].vt & ~VT_BYREF) == vt)
			{
				param = pDispParams->rgvarg[arg];
				hr = S_OK;
			}
			else if (puArgErr)
			{
				// Its really an error
				*puArgErr = arg;
			}
		}
		if FAILED(hr)
			return hr;

		// Put into param vector
		vecParams.push_back(std::pair<CComVariant,OOCore::TypeInfo::Type_t>(param,type));
	}

	// Call CreateRequest for an OutputStream
	OOObject::uint32_t trans_id;
	OOCore::Object_Ptr<OOCore::OutputStream> ptrRequest;
	if (INTEROP::instance()->CreateRequest(attr,m_OO_key,dispIdMember,&trans_id,&ptrRequest) != 0)
		return E_UNEXPECTED;

	// Write out each param
	for (std::vector<std::pair<CComVariant,OOCore::TypeInfo::Type_t> >::iterator k=vecParams.begin();k!=vecParams.end();++k)
	{
		if (k->second & OOCore::TypeInfo::in)
		{
            // Write the data to the outputstream
			hr = WriteParam(k->first,k->second,ptrRequest);
			if FAILED(hr)
			{
				INTEROP::instance()->CancelRequest(trans_id);
				return hr;
			}
		}
	}

	// Invoke
	CComVariant ret_code(0,VT_I4);
	OOCore::Object_Ptr<OOCore::InputStream> ptrResponse;
	ret_code.lVal = INTEROP::instance()->Invoke(attr,wait_secs,ptrRequest,trans_id,&ptrResponse);

	if (pVarResult)
		*pVarResult = ret_code;

	if (ret_code.lVal != 0)
		return E_UNEXPECTED;

	// If we want a response
	if (!(attr & OOCore::TypeInfo::async_method))
	{
		// Read Params
		for (k=vecParams.begin();k!=vecParams.end();++k)
		{
			if (k->second & OOCore::TypeInfo::out)
			{
				// Write the data to the outputstream
				hr = ReadParam(&k->first,k->second,ptrResponse);
				if FAILED(hr)
					return hr;
			}
		}
	}

	return S_OK;
}
