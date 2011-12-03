///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOCOMInterop, the Omega Online COM Interop library.
//
// OOCOMInterop is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCOMInterop is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCOMInterop.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "disp_obj.h"

namespace
{
	HRESULT parse_type_info(Omega::Remoting::IMessage* pType, bool& seen_array, Omega::guid_t& iid, Omega::TypeInfo::Type_t& type)
	{
		bool seen_pointer = false;
		for (;;)
		{
			type = pType->ReadValue(L"type").cast<Omega::TypeInfo::Type_t>();
			switch (type)
			{
				case Omega::TypeInfo::typeVoid:
					// No voids
					return DISP_E_TYPEMISMATCH;
				
				case Omega::TypeInfo::typeBool:	
				case Omega::TypeInfo::typeByte:			
				case Omega::TypeInfo::typeInt16:			
				case Omega::TypeInfo::typeUInt16:			
				case Omega::TypeInfo::typeInt32:			
				case Omega::TypeInfo::typeUInt32:			
				case Omega::TypeInfo::typeInt64:
				case Omega::TypeInfo::typeUInt64:				
				case Omega::TypeInfo::typeFloat4:			
				case Omega::TypeInfo::typeFloat8:		
				case Omega::TypeInfo::typeString:
				case Omega::TypeInfo::typeAny:
				case Omega::TypeInfo::typeGuid:
					if (seen_pointer)
					{
						if (seen_array)
							return DISP_E_TYPEMISMATCH; // No collections of arrays
						
						seen_array = true;
					}					
					return S_OK;
				
				case Omega::TypeInfo::typeObject:
					if (!seen_pointer)
						return DISP_E_TYPEMISMATCH; // Must be an Omega::IObject*
					
					iid = pType->ReadValue(L"iid").cast<Omega::guid_t>();
					return S_OK;
				
				case Omega::TypeInfo::modifierPointer:
					if (seen_pointer)
						return DISP_E_TYPEMISMATCH; // No TYPE** support
					seen_pointer = true;
					break;
					
				case Omega::TypeInfo::typeSTLVector:
				case Omega::TypeInfo::typeSTLDeque:
				case Omega::TypeInfo::typeSTLList:
				case Omega::TypeInfo::typeSTLSet:
				case Omega::TypeInfo::typeSTLMultiset:
					if (seen_pointer)
						return DISP_E_TYPEMISMATCH; // Must be an array and no stl::<>* support
					
					type = Omega::TypeInfo::typeVoid;
					seen_array = true;
					break;
					
				case Omega::TypeInfo::modifierReference:
				case Omega::TypeInfo::modifierConst:
					// Don't care about modifiers
					type = Omega::TypeInfo::typeVoid;
					break;
				
				case Omega::TypeInfo::typeSTLMap:
				case Omega::TypeInfo::typeSTLMultimap:
				default:
					// We don't support 2D collections
					return DISP_E_TYPEMISMATCH;
			}				
		}
	}
}

HRESULT variant_to_any(LCID lcid, const VARIANTARG& arg, Omega::any_t& value)
{
	switch (arg.vt)
	{
	case VT_I8:
		value = Omega::any_t(arg.llVal);
		break;

	case VT_I4:
		value = Omega::any_t(static_cast<Omega::int32_t>(arg.lVal));
		break;

	case VT_UI1:
		value = Omega::any_t(arg.bVal);
		break;

	case VT_I2:
		value = Omega::any_t(arg.iVal);
		break;

	case VT_R4:
		value = Omega::any_t(arg.fltVal);
		break;

	case VT_R8:
		value = Omega::any_t(arg.dblVal);
		break;

	case VT_BOOL:
		value = Omega::any_t(arg.boolVal == VARIANT_TRUE);
		break;

	case VT_ERROR:
		if (arg.scode != DISP_E_PARAMNOTFOUND)
			return DISP_E_TYPEMISMATCH;
		value = Omega::any_t();
		break;

	case VT_DATE:
		{
			VARIANTARG arg2;
			HRESULT hr = VariantChangeTypeEx(&arg2,const_cast<VARIANTARG*>(&arg),lcid,0,VT_BSTR);
			if (hr != S_OK)
				return hr;
			
			return variant_to_any(lcid,arg2,value);
		}

	case VT_BSTR:
		value = Omega::any_t(arg.bstrVal);
		break;

	case VT_BYREF|VT_UI1:
		if (!arg.pbVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pbVal);
		break;

	case VT_BYREF|VT_I2:
		if (!arg.piVal)
			return E_POINTER;
		value = Omega::any_t(*arg.piVal);
		break;

	case VT_BYREF|VT_I4:
		if (!arg.plVal)
			return E_POINTER;
		value = Omega::any_t(static_cast<Omega::uint32_t>(*arg.plVal));
		break;

	case VT_BYREF|VT_I8:
		if (!arg.pllVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pllVal);
		break;

	case VT_BYREF|VT_R4:
		if (!arg.pfltVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pfltVal);
		break;

	case VT_BYREF|VT_R8:
		if (!arg.pdblVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pdblVal);
		break;

	case VT_BYREF|VT_BOOL:
		if (!arg.pboolVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pboolVal == VARIANT_TRUE);
		break;
	
	case VT_BYREF|VT_ERROR:
		if (!arg.pscode)
			return E_POINTER;
		else if (*arg.pscode != DISP_E_PARAMNOTFOUND)
			return DISP_E_TYPEMISMATCH;
		value = Omega::any_t();
		break;

	case VT_BYREF|VT_DATE:
		if (!arg.pdate)
			return E_POINTER;
		else
		{
			VARIANTARG arg2;
			HRESULT hr = VariantChangeTypeEx(&arg2,const_cast<VARIANTARG*>(&arg),lcid,0,VT_BSTR);
			if (hr != S_OK)
				return hr;
			
			return variant_to_any(lcid,arg2,value);
		}

	case VT_BYREF|VT_BSTR:
		if (!arg.pbstrVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pbstrVal);
		break;

	case VT_BYREF|VT_VARIANT:
		if (!arg.pvarVal)
			return E_POINTER;
		else if (arg.pvarVal->vt == (VT_BYREF|VT_VARIANT))
			return DISP_E_TYPEMISMATCH;
		return variant_to_any(lcid,*arg.pvarVal,value);
		
	case VT_I1:
		value = Omega::any_t(arg.cVal);
		break;

	case VT_UI2:
		value = Omega::any_t(arg.uiVal);
		break;

	case VT_UI4:
		value = Omega::any_t(static_cast<Omega::uint32_t>(arg.ulVal));
		break;

	case VT_UI8:
		value = Omega::any_t(arg.ullVal);
		break;

	case VT_INT:
		value = Omega::any_t(arg.intVal);
		break;

	case VT_UINT:
		value = Omega::any_t(arg.uintVal);
		break;

	case VT_BYREF|VT_I1:
		if (!arg.pcVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pcVal);
		break;

	case VT_BYREF|VT_UI2:
		if (!arg.puiVal)
			return E_POINTER;
		value = Omega::any_t(*arg.puiVal);
		break;

	case VT_BYREF|VT_UI4:
		if (!arg.pulVal)
			return E_POINTER;
		value = Omega::any_t(static_cast<Omega::uint32_t>(*arg.pulVal));
		break;

	case VT_BYREF|VT_UI8:
		if (!arg.pullVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pullVal);
		break;

	case VT_BYREF|VT_INT:
		if (!arg.pintVal)
			return E_POINTER;
		value = Omega::any_t(*arg.pintVal);
		break;

	case VT_BYREF|VT_UINT:
		if (!arg.puintVal)
			return E_POINTER;
		value = Omega::any_t(*arg.puintVal);
		break;
	
	default:
		return DISP_E_TYPEMISMATCH;
	}
	return S_OK;
}	

void any_to_variant(LCID lcid, const Omega::any_t& value, VARIANTARG* pArg)
{
	void* TODO;
}

IDispatchObjImpl::IDispatchObjImpl(Omega::Remoting::IProxy* pProxy, const Omega::guid_t& iid) : 
		IDispatchImpl(),
		m_iid(iid),
		m_ptrProxy(pProxy)		
{ 
	m_ptrProxy.AddRef();
}

IDispatchObjImpl::~IDispatchObjImpl()
{ }

HRESULT IDispatchObjImpl::init()
{
	if (!m_ptrProxy)
		return E_INVALIDARG;
	
	try
	{
		m_ptrInfo = Omega::TypeInfo::GetInterfaceInfo(m_iid,m_ptrProxy);
		if (!m_ptrInfo)
			return E_INVALIDARG;
		
		m_ptrMarshaller = m_ptrProxy->GetMarshaller();
		if (!m_ptrMarshaller)
			return E_INVALIDARG;
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		return E_NOINTERFACE;
	}
		
	return S_OK;
}

STDMETHODIMP IDispatchObjImpl::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID /*lcid*/, DISPID* rgDispId)
{
	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;
	
	if (!rgszNames || !rgDispId)
		return E_POINTER;
	
	if (cNames == 0)
		return E_INVALIDARG;
	
	try
	{
		Omega::byte_t param_count = 0;
		Omega::uint32_t method = 2;
		
		if (cNames >= 1)
		{
			// Get the method
			Omega::uint32_t methodCount = m_ptrInfo->GetMethodCount();
						
			// Always Add 2 to avoid IObject::AddRef and IObject::Release
			for (; method < methodCount; ++method)
			{
				Omega::TypeInfo::MethodAttributes_t attribs;
				Omega::uint32_t timeout;
				Omega::string_t strName;
				Omega::Remoting::IMessage* return_type = NULL;
								
				m_ptrInfo->GetMethodInfo(method,strName,attribs,timeout,param_count,return_type);
					
				if (return_type)
					return_type->Release();
				
				if (strName == rgszNames[0])
					break;
			}
			
			if (method == methodCount)
				return DISP_E_UNKNOWNNAME;
			
			rgDispId[0] = method - 2;			
		}
		
		for (UINT idx = 1; idx < cNames; ++idx)
		{
			Omega::byte_t param_idx = 0;
			for (; param_idx < param_count; ++param_idx)
			{
				Omega::string_t strName;
				Omega::Remoting::IMessage* type = NULL;
				Omega::TypeInfo::ParamAttributes_t attribs;
				
				m_ptrInfo->GetParamInfo(method,param_idx,strName,type,attribs);
				
				if (type)
					type->Release();
				
				if (strName == rgszNames[idx])
				{
					rgDispId[idx] = param_idx;		
					break;
				}
			}
			
			if (param_idx == param_count)
				return DISP_E_UNKNOWNNAME;			
		}	
	}
	catch (Omega::IException* pE)
	{
		pE->Release();
		return E_UNEXPECTED;
	}
	
	return S_OK;
}

STDMETHODIMP IDispatchObjImpl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;
		
	if (!(wFlags & DISPATCH_METHOD))
		return DISP_E_MEMBERNOTFOUND;
	
	if (!pExcepInfo)
		return E_POINTER;
	
	// Always +2 to avoid IObject::AddRef and IObject::Release
	dispIdMember += 2;
		
	// Get the member info
	Omega::byte_t param_count;
	Omega::TypeInfo::MethodAttributes_t attribs;
	Omega::uint32_t timeout;
	Omega::string_t strName;
	
	OTL::ObjectPtr<Omega::Remoting::IMessage> ptrRetType;
	try
	{
		// Always Add 2 to avoid IObject::AddRef and IObject::Release
		m_ptrInfo->GetMethodInfo(dispIdMember,strName,attribs,timeout,param_count,ptrRetType);
	}
	catch (Omega::IException* pE)	
	{
		pE->Release();
		return DISP_E_MEMBERNOTFOUND;
	}
	
	// Check parameter count
	if (param_count != 0)
	{
		if (!pDispParams)
			return E_POINTER;
		
		if (pDispParams->cArgs > param_count)
			return DISP_E_BADPARAMCOUNT;
		
		// Check the named arguments
		for (UINT idx = pDispParams->cNamedArgs; idx > 0; --idx)
		{
			// Check each DISPID < param_count, excluding the unnamed args (that go first)
			if (static_cast<UINT>(pDispParams->rgdispidNamedArgs[idx-1]) >= param_count - (pDispParams->cArgs - pDispParams->cNamedArgs))
			{
				if (puArgErr)
					*puArgErr = idx-1;
				
				return DISP_E_PARAMNOTFOUND;
			}
		}
	}
	else
	{
		if (pDispParams && pDispParams->cArgs > param_count)
			return DISP_E_BADPARAMCOUNT;
	}
	
	OTL::ObjectPtr<Omega::Remoting::IMessage> ptrResult;
	Omega::byte_t params_written = 0;
	OTL::ObjectPtr<Omega::Remoting::IMessage> ptrMsg;
	
	try
	{
		// Now build the message
		ptrMsg = m_ptrMarshaller->CreateMessage();
	}
	catch (Omega::IException* pE)
	{
		return FillExcepInfo(strName.c_wstr(),pE,pExcepInfo);
	}
		
	try
	{
		// Write the magic prolog
		ptrMsg->WriteStructStart(L"ipc_request",L"$ipc_request_type");
		m_ptrProxy->WriteKey(ptrMsg);
		ptrMsg->WriteValue(L"$iid",m_iid);
		ptrMsg->WriteValue(L"$method_id",static_cast<Omega::uint32_t>(dispIdMember));
						
		// See http://msdn.microsoft.com/en-us/library/ms221653.aspx for details of DISPPARAMS
		// Key points: args are in reverse order (right to left)
		//             unnamed args processed before named args
		if (pDispParams)
		{
			UINT arg = pDispParams->cArgs;
			for (; arg > pDispParams->cNamedArgs; --arg, ++params_written)
			{
				HRESULT hr = WriteArg(lcid,ptrMsg,dispIdMember,params_written,pDispParams->rgvarg[arg-1]);
				if (hr != S_OK)
				{
					if (puArgErr)
						*puArgErr = arg-1;
				
					UnpackArgs(lcid,ptrMsg,dispIdMember,params_written);
					return hr;
				}
			}
		
			// Now write named args
			for (; arg > 0; --arg, ++params_written)
			{
				UINT idx = 0;
				for (; idx < pDispParams->cNamedArgs; ++idx)
				{
					if (pDispParams->rgdispidNamedArgs[idx] == static_cast<INT>(arg-1))
						break;
				}
			
				if (idx != pDispParams->cNamedArgs)
				{
					HRESULT hr = WriteArg(lcid,ptrMsg,dispIdMember,params_written,pDispParams->rgvarg[arg-1]);
					if (hr != S_OK)
					{
						if (puArgErr)
							*puArgErr = arg-1;
					
						UnpackArgs(lcid,ptrMsg,dispIdMember,params_written);
						return hr;
					}
				}
				else
					WriteDefault(lcid,ptrMsg,dispIdMember,params_written);			
			}
		}
		
		// Now write missing params as any_t()
		while (params_written < param_count)
		{
			WriteDefault(lcid,ptrMsg,dispIdMember,params_written);			
			++params_written;
		}
		
		ptrMsg->WriteStructEnd();
			
		// Now make the call...
		Omega::IException* pE = m_ptrMarshaller->SendAndReceive(attribs,ptrMsg,ptrResult,timeout);
		if (pE)
			return FillExcepInfo(strName.c_wstr(),pE,pExcepInfo);			
	}
	catch (Omega::IException* pE)
	{
		// This exception occurred setting up the message
		try
		{
			UnpackArgs(lcid,ptrMsg,dispIdMember,params_written);
		}
		catch (Omega::IException* pE2)
		{
			pE2->Release();
		}
		
		return FillExcepInfo(strName.c_wstr(),pE,pExcepInfo);
	}
	
	// Read results...
	try
	{
		// Read $retval
		ReadArgByType(lcid,L"$retval",ptrMsg,ptrRetType,pVarResult);
		
		Omega::byte_t params_read = 0;
		if (pDispParams)
		{
			UINT arg = pDispParams->cArgs;
			for (; arg > pDispParams->cNamedArgs; ++params_read,--arg)
			{
				ReadArg(lcid,ptrMsg,dispIdMember,params_read,&pDispParams->rgvarg[arg-1]);
			}
		
			// Now read named args
			for (;arg > 0; ++params_read,--arg)
			{
				UINT idx = 0;
				for (; idx < pDispParams->cNamedArgs; ++idx)
				{
					if (pDispParams->rgdispidNamedArgs[idx] == static_cast<INT>(arg-1))
						break;
				}
			
				if (idx != pDispParams->cNamedArgs)
					ReadArg(lcid,ptrMsg,dispIdMember,params_read,&pDispParams->rgvarg[arg-1]);
				else
					ReadArg(lcid,ptrMsg,dispIdMember,params_read,NULL);
			}
		}
		
		// Now read missing params
		while (params_read < param_count)
		{
			ReadArg(lcid,ptrMsg,dispIdMember,params_read,NULL);
			++params_read;
		}
	}
	catch (Omega::IException* pE)
	{
		return FillExcepInfo(strName.c_wstr(),pE,pExcepInfo);
	}
		
	return S_OK;	
}

void IDispatchObjImpl::WriteDefault(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param)
{
	SCODE v = DISP_E_PARAMNOTFOUND;
	VARIANTARG arg;
	arg.vt = VT_BYREF | VT_ERROR;
	arg.pscode = &v;
	
	WriteArg(lcid,pMsg,method,param,arg);
}

HRESULT IDispatchObjImpl::WriteArg(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param, const VARIANTARG& arg)
{
	Omega::string_t strName;
	Omega::TypeInfo::ParamAttributes_t attribs;
	OTL::ObjectPtr<Omega::Remoting::IMessage> ptrType;
	
	m_ptrInfo->GetParamInfo(method,param,strName,ptrType,attribs);
		
	if (!(arg.vt & VT_BYREF) && (attribs & Omega::TypeInfo::attrOut))
		return DISP_E_TYPEMISMATCH;  // [out] params must be ByRef
	
	bool seen_array = false;
	Omega::guid_t iid = Omega::guid_t::Null();
	Omega::TypeInfo::Type_t type = Omega::TypeInfo::typeVoid;
	
	// Check all types now!
	HRESULT hr = parse_type_info(ptrType,seen_array,iid,type);
	if (hr != S_OK)
		return hr;
	
	if (type == Omega::TypeInfo::typeObject)
	{
		void* TODO; //  Add IDispatch stubs one day maybe?
		return E_NOTIMPL;
	}
	else if (seen_array)
	{
		void* TODO; // // Add array support?
		return E_NOTIMPL;
	}
		
	if (attribs & Omega::TypeInfo::attrIn)
	{
		Omega::any_t value;
		hr = variant_to_any(lcid,arg,value);
		if (hr != S_OK)
			return hr;
		
		pMsg->WriteValue(strName,value);			
	}
		
	return S_OK;	
}

void IDispatchObjImpl::ReadArgByType(LCID lcid, const Omega::string_t& strName, Omega::Remoting::IMessage* pMsg, Omega::Remoting::IMessage* pType, VARIANTARG* pArg)
{
	bool seen_array = false;
	Omega::guid_t iid = Omega::guid_t::Null();
	Omega::TypeInfo::Type_t type = Omega::TypeInfo::typeVoid;
	
	VariantClear(pArg);
	
	parse_type_info(pType,seen_array,iid,type);
	
	if (type == Omega::TypeInfo::typeObject)
	{
		void* TODO; //  Add IDispatch stubs one day maybe?
	}
	else if (seen_array)
	{
		void* TODO; // // Add array support?
	}
	else
	{	
		Omega::any_t value = pMsg->ReadValue(strName);
		if (pArg)
			any_to_variant(lcid,value,pArg);
	}
}

void IDispatchObjImpl::ReadArg(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param, VARIANTARG* pArg)
{
	Omega::string_t strName;
	Omega::TypeInfo::ParamAttributes_t attribs;
	OTL::ObjectPtr<Omega::Remoting::IMessage> ptrType;
	
	m_ptrInfo->GetParamInfo(method,param,strName,ptrType,attribs);
	
	if (attribs & Omega::TypeInfo::attrOut)
		ReadArgByType(lcid,strName,pMsg,ptrType,pArg);
}

void IDispatchObjImpl::UnpackArgs(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param_count)
{
	for (Omega::byte_t param = 0; param < param_count; ++param)
		ReadArg(lcid,pMsg,method,param,NULL);
}
