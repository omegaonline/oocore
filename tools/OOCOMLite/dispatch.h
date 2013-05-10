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

#include "../../include/Omega/Omega.h"
#include "../../include/Omega/Compartment.h"
#include "../../include/Omega/Remoting.h"
#include "../../include/OTL/OTL.h"

#include <objbase.h>
#include <oleauto.h>

#if !defined(DISPATCH_H_INCLUDED_)
#define DISPATCH_H_INCLUDED_

HRESULT variant_to_any(LCID lcid, const VARIANTARG& arg, Omega::any_t& value);
void any_to_variant(LCID lcid, const Omega::any_t& value, VARIANTARG* pArg);
HRESULT FillExcepInfo(const Omega::string_t& strSource, Omega::IException* pE, EXCEPINFO* pExcepInfo);
BSTR ToBSTR(const Omega::string_t& str);
Omega::string_t FromBSTR(const OLECHAR* bstr);

class IDispatchImpl : public IDispatch
{
protected:
	IDispatchImpl();
	virtual ~IDispatchImpl();

private:
	IDispatchImpl(const IDispatchImpl&);
	IDispatchImpl& operator = (const IDispatchImpl&);

	LONG m_refCount;
	
// IDispatch members
public:
	STDMETHOD_(ULONG,AddRef)()
	{
		return InterlockedIncrement(&m_refCount);
	}
	
	STDMETHOD_(ULONG,Release)()
	{
		ULONG ret = InterlockedDecrement(&m_refCount);
		if (ret == 0)
			delete this;
		return ret;
	}

	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)
	{
		if (!ppvObject)
			return E_POINTER;
		
		*ppvObject = NULL;

		if (riid == IID_IUnknown || riid == IID_IDispatch)
		{
			*ppvObject = this;
			AddRef();
			return S_OK;
		}
	
		return E_NOINTERFACE;
	}

	STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
	{
		if (!pctinfo)
			return E_INVALIDARG;
		
		*pctinfo = 0;
		return S_OK;
	}

	STDMETHOD(GetTypeInfo)(UINT /*iTInfo*/, LCID /*lcid*/, ITypeInfo** ppTInfo)
	{
		if (!ppTInfo)
			return E_POINTER;
		
		*ppTInfo = NULL;
		return DISP_E_BADINDEX;		
	}
	
	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) = 0;
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) = 0;
};

#endif // DISPATCH_H_INCLUDED_
