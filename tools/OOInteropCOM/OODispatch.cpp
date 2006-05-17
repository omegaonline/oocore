// OODispatch.cpp : Implementation of COODispatch

#include "stdafx.h"
#include "OODispatch.h"
#include ".\oodispatch.h"

// COODispatch
HRESULT COODispatch::init(const OOObject::uint32_t& key, OOUtil::Object_Ptr<OOObject::TypeInfo>& type_info)
{
	m_OO_key = key;
	m_ptr_OO_TypeInfo = type_info;

	return S_OK;
}

HRESULT COODispatch::ReadParam(VARIANT* var, OOObject::TypeInfo::Type_t type, OOUtil::InputStream_Ptr& ptrIn)
{
	/*VT_ARRAY	= 0x2000*/
	
	switch (var->vt)
	{
	case VT_DISPATCH:
	case VT_UNKNOWN:

	case VT_I8:
		return Read(var->llVal,ptrIn);

    case VT_I4:
		{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Read(v,ptrIn)) == S_OK)
				var->lVal = v;
			return hr;
		}

    case VT_UI1:
		return Read(var->bVal,ptrIn);

    case VT_I2:
		return Read(var->iVal,ptrIn);

    case VT_R4:
		return Read(var->fltVal,ptrIn);

    case VT_R8:
		return Read(var->dblVal,ptrIn);

    case VT_BOOL:
		return Read(var->boolVal,ptrIn);
    
	case (VT_BYREF | VT_UI1):
		return Read(*var->pbVal,ptrIn);

	case (VT_BYREF | VT_I2):
		return Read(*var->piVal,ptrIn);
    
	case (VT_BYREF | VT_I4):
		{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Read(v,ptrIn)) == S_OK)
				*var->plVal = v;
			return hr;
		}

	case (VT_BYREF | VT_I8):
		return Read(*var->pllVal,ptrIn);
    
	case (VT_BYREF | VT_R4):
		return Read(*var->pfltVal,ptrIn);

	case (VT_BYREF | VT_R8):
		return Read(*var->pdblVal,ptrIn);

	case (VT_BYREF | VT_BOOL):
		return Read(*var->pboolVal,ptrIn);
    
    case VT_BSTR:
		if (type & OOObject::TypeInfo::string)
			return ReadString(var->bstrVal,type,ptrIn);
		else
			return DISP_E_BADVARTYPE;		

	case (VT_BYREF | VT_BSTR):
		if (type & OOObject::TypeInfo::string)
			return ReadString(*var->pbstrVal,type,ptrIn);
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
		return Read(var->cVal,ptrIn);

    case VT_UI2:
		return Read(var->uiVal,ptrIn);

    case VT_UI4:
		{
			// This is bust for some reason!
			OOObject::uint32_t v;
			HRESULT hr;
			if ((hr=Read(v,ptrIn)) == S_OK)
				var->ulVal = v;
			return hr;
		}

    case VT_UI8:
		return Read(var->ullVal,ptrIn);

    case VT_INT:
		return Read(var->intVal,ptrIn);

    case VT_UINT:
		return Read(var->uintVal,ptrIn);

	case (VT_BYREF | VT_I1):
		return Read(*var->pcVal,ptrIn);

	case (VT_BYREF | VT_UI2):
		return Read(*var->puiVal,ptrIn);

	case (VT_BYREF | VT_UI4):
		{
			// This is bust for some reason!
			OOObject::uint32_t v;
			HRESULT hr;
			if ((hr=Read(v,ptrIn)) == S_OK)
				*var->pulVal = v;
			return hr;
		}

    case (VT_BYREF | VT_UI8):
		return Read(*var->pullVal,ptrIn);

    case (VT_BYREF | VT_INT):
		return Read(*var->pintVal,ptrIn);

    case (VT_BYREF | VT_UINT):
		return Read(*var->puintVal,ptrIn);
    	
	// We can't handle anything else!
	default:
		return DISP_E_BADVARTYPE;
	}
}

HRESULT COODispatch::ReadString(BSTR& bstr, OOObject::TypeInfo::Type_t type, OOUtil::InputStream_Ptr& in)
{
	::SysFreeString(bstr);
	bstr=NULL;

	OOObject::uint32_t nLen = 0;
	HRESULT hr = Read(nLen,in);
	if FAILED(hr)
		return hr;

	if (nLen==0)
		return S_OK;

	if (type & OOObject::TypeInfo::char_t)
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
	else if (type & OOObject::TypeInfo::int16_t)
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

HRESULT COODispatch::WriteParam(const CComVariant& var, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& ptrOut)
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
		return Write(var.llVal,ptrOut);

    case VT_I4:
		return Write(var.lVal,ptrOut);
		/*{
			// This is bust for some reason!
			OOObject::int32_t v;
			HRESULT hr;
			if ((hr=Write(v,ptrOut)) == S_OK)
				var->lVal = v;
			return hr;
		}*/

    case VT_UI1:
		return Write(var.bVal,ptrOut);

    case VT_I2:
		return Write(var.iVal,ptrOut);

    case VT_R4:
		return Write(var.fltVal,ptrOut);

    case VT_R8:
		return Write(var.dblVal,ptrOut);

    case VT_BOOL:
		return Write(var.boolVal,ptrOut);
    
	case (VT_BYREF | VT_UI1):
		return Write(*var.pbVal,ptrOut);

	case (VT_BYREF | VT_I2):
		return Write(*var.piVal,ptrOut);
    
	case (VT_BYREF | VT_I4):
		return Write(*var.plVal,ptrOut);
		
	case (VT_BYREF | VT_I8):
		return Write(*var.pllVal,ptrOut);
    
	case (VT_BYREF | VT_R4):
		return Write(*var.pfltVal,ptrOut);

	case (VT_BYREF | VT_R8):
		return Write(*var.pdblVal,ptrOut);

	case (VT_BYREF | VT_BOOL):
		return Write(*var.pboolVal,ptrOut);
    
    case VT_BSTR:
		if (type & OOObject::TypeInfo::string)
			return WriteString(var.bstrVal,type,ptrOut);
		else
			return DISP_E_BADVARTYPE;		

	case (VT_BYREF | VT_BSTR):
		if (type & OOObject::TypeInfo::string)
			return WriteString(*var.pbstrVal,type,ptrOut);
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
		return Write(var.cVal,ptrOut);

    case VT_UI2:
		return Write(var.uiVal,ptrOut);

    case VT_UI4:
		return Write(static_cast<OOObject::uint32_t>(var.ulVal),ptrOut);

    case VT_UI8:
		return Write(var.ullVal,ptrOut);

    case VT_INT:
		return Write(var.intVal,ptrOut);

    case VT_UINT:
		return Write(var.uintVal,ptrOut);

	case (VT_BYREF | VT_I1):
		return Write(*var.pcVal,ptrOut);

	case (VT_BYREF | VT_UI2):
		return Write(*var.puiVal,ptrOut);

	case (VT_BYREF | VT_UI4):
		return Write(static_cast<OOObject::int32_t>(*var.pulVal),ptrOut);

    case (VT_BYREF | VT_UI8):
		return Write(*var.pullVal,ptrOut);

    case (VT_BYREF | VT_INT):
		return Write(*var.pintVal,ptrOut);

    case (VT_BYREF | VT_UINT):
		return Write(*var.puintVal,ptrOut);
    	
	// We can't handle anything else!
	default:
		return DISP_E_BADVARTYPE;
	}
}

HRESULT COODispatch::WriteArray(SAFEARRAY* parray, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& ptrOut)
{
	long ubound,lbound;

	// Remove the array tag from type
	type &= ~OOObject::TypeInfo::array;

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

HRESULT COODispatch::WriteString(const BSTR bstr, OOObject::TypeInfo::Type_t type, OOUtil::OutputStream_Ptr& out)
{
	OOObject::uint32_t nLen = ::SysStringLen(bstr);
	HRESULT hr = Write(nLen,out);
	if FAILED(hr)
		return hr;

	if (type & OOObject::TypeInfo::char_t)
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
	else if (type & OOObject::TypeInfo::int16_t)
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

VARTYPE COODispatch::OOTypeToVARTYPE(OOObject::TypeInfo::Type_t type)
{
	// Work out the basic type
	VARTYPE result = VT_EMPTY;
	switch (type & OOObject::TypeInfo::TYPE_MASK)
	{
	case OOObject::TypeInfo::bool_t:
		result = VT_BOOL;
		break;

	case OOObject::TypeInfo::char_t:
		result = VT_I1;
		break;

	case OOObject::TypeInfo::byte_t:
		result = VT_UI1;
		break;

	case OOObject::TypeInfo::int16_t:
		result = VT_I2;
		break;

	case OOObject::TypeInfo::uint16_t:
		result = VT_UI2;
		break;

	case OOObject::TypeInfo::int32_t:
		result = VT_I4;
		break;

	case OOObject::TypeInfo::uint32_t:
		result = VT_UI4;
		break;

	case OOObject::TypeInfo::int64_t:
		result = VT_I8;
		break;

	case OOObject::TypeInfo::uint64_t:
		result = VT_UI8;
		break;

	case OOObject::TypeInfo::real4_t:
		result = VT_R4;
		break;

	case OOObject::TypeInfo::real8_t:
		result = VT_R8;
		break;

	case OOObject::TypeInfo::Object:	
		result = VT_DISPATCH;
		break;

	// The other values make no sense in IDispatch world
	default:
		break;
	}

	OOObject::TypeInfo::Type_t attr_type = (type & OOObject::TypeInfo::ATTR_MASK);

	// Check for possible string types...
	if (attr_type & OOObject::TypeInfo::string)
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
	if (attr_type & OOObject::TypeInfo::out)
	{
		// Must be a pointer
		result |= VT_BYREF;
	}

	if (attr_type & OOObject::TypeInfo::array)
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
		OOObject::TypeInfo::Method_Attributes_t attribs;
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
		OOObject::TypeInfo::Type_t type;
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
	OOObject::TypeInfo::Method_Attributes_t attr;
	size_t param_count;
	OOObject::uint16_t wait_secs;
	if (m_ptr_OO_TypeInfo->GetMethodInfo(dispIdMember,&name,&param_count,&attr,&wait_secs) != 0)
		return DISP_E_MEMBERNOTFOUND;

	// Enumerate params
	HRESULT hr;
	std::vector<std::pair<CComVariant,OOObject::TypeInfo::Type_t> > vecParams;
	for (size_t p=0;p<param_count;++p)
	{
		// Get the param type info
		OOObject::TypeInfo::Type_t type;
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
		vecParams.push_back(std::pair<CComVariant,OOObject::TypeInfo::Type_t>(param,type));
	}

	// Call CreateRequest for an OutputStream
	OOObject::uint32_t trans_id;
	OOUtil::OutputStream_Ptr ptrRequest;
	if (INTEROP::instance()->CreateRequest(dispIdMember,attr,m_OO_key,&trans_id,&ptrRequest) != 0)
		return E_UNEXPECTED;

	// Write out each param
	for (std::vector<std::pair<CComVariant,OOObject::TypeInfo::Type_t> >::iterator k=vecParams.begin();k!=vecParams.end();++k)
	{
		if (k->second & OOObject::TypeInfo::in)
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
	OOUtil::InputStream_Ptr ptrResponse;
	ret_code.lVal = INTEROP::instance()->Invoke(attr,wait_secs,ptrRequest,trans_id,&ptrResponse);

	if (pVarResult)
		*pVarResult = ret_code;

	if (ret_code.lVal != 0)
		return E_UNEXPECTED;

	// If we want a response
	if (!(attr & OOObject::TypeInfo::async_method))
	{
		// Read Params
		for (k=vecParams.begin();k!=vecParams.end();++k)
		{
			if (k->second & OOObject::TypeInfo::out)
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
