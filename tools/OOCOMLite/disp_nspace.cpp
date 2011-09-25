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

#include "disp_nspace.h"

STDMETHODIMP IDispatchNSpaceImpl::GetIDsOfNames(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID /*lcid*/, DISPID* rgDispId)
{
	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;
	
	if (!rgszNames || !rgDispId)
		return E_POINTER;
	
	if (cNames == 0)
		return E_INVALIDARG;
	
	for (DISPID id = 0; m_pDefn[id].pszNames != NULL; ++id)
	{
		if (wcscmp(rgszNames[0],m_pDefn[id].pszNames[0]) == 0)
		{
			for (UINT param = 1; param < cNames; ++param)
			{
				UINT p = 1;
				for (; m_pDefn[id].pszNames[p] != NULL; ++p)
				{
					if (wcscmp(rgszNames[param],m_pDefn[id].pszNames[p]) == 0)
					{
						rgDispId[param] = p;
						break;
					}
				}
					
				if (m_pDefn[id].pszNames[p] == NULL)
					return DISP_E_UNKNOWNNAME;
			}
			
			rgDispId[0] = id;
			return S_OK;
		}
	}
		
	return DISP_E_UNKNOWNNAME;		
}

STDMETHODIMP IDispatchNSpaceImpl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
	if (riid != IID_NULL)
		return DISP_E_UNKNOWNINTERFACE;
	
	if (!pExcepInfo)
		return E_POINTER;
	
	try
	{
		if ((wFlags & DISPATCH_PROPERTYGET) && m_pDefn[dispIdMember].pfnGet)
		{
			if (!pVarResult)
				return E_POINTER;
				
			HRESULT hr = VariantClear(pVarResult);
			if (hr != S_OK)
				return hr;
				
			if (pDispParams && pDispParams->cArgs != 0)
				return DISP_E_BADPARAMCOUNT;
				
			return (*m_pDefn[dispIdMember].pfnGet)(lcid,pVarResult,pExcepInfo);
		}
		
		if (wFlags & (DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF))
		{
			if (!pDispParams)
				return E_POINTER;
				
			if (pDispParams->cArgs != 1)
				return DISP_E_BADPARAMCOUNT;
				
			if (!m_pDefn[dispIdMember].pfnPut)
				return DISP_E_MEMBERNOTFOUND;
			
			return (*m_pDefn[dispIdMember].pfnPut)(lcid,pDispParams,puArgErr,pExcepInfo,(wFlags & DISPATCH_PROPERTYPUTREF) == DISPATCH_PROPERTYPUTREF);
		}
		
		if (!m_pDefn[dispIdMember].pfnInvoke)
			return DISP_E_MEMBERNOTFOUND;
				
		return (*m_pDefn[dispIdMember].pfnInvoke)(lcid,pDispParams,pVarResult,puArgErr,pExcepInfo);	
	}
	catch (Omega::IException* pE)
	{
		return FillExcepInfo(m_pDefn[dispIdMember].pszNames[0],pE,pExcepInfo);
	}
}
