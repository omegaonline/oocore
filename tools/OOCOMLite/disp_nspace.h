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

#include "dispatch.h"

struct NSpaceDefn
{
	const OLECHAR** pszNames;
	HRESULT (*pfnInvoke)(LCID lcid, DISPPARAMS* pDispParams, VARIANT* pVarResult, UINT* puArgErr, EXCEPINFO* pExcepInfo);
	HRESULT (*pfnGet)(LCID lcid, VARIANT* pVarResult, EXCEPINFO* pExcepInfo);
	HRESULT (*pfnPut)(LCID lcid, DISPPARAMS* pDispParams, UINT* puArgErr, EXCEPINFO* pExcepInfo, bool byRef);
};

class IDispatchNSpaceImpl : public IDispatchImpl
{
public:
	IDispatchNSpaceImpl(const NSpaceDefn* pDefn) : 
		IDispatchImpl(),
		m_pDefn(pDefn)
	{ }

private:
	const NSpaceDefn* m_pDefn;
	
// IDispatch members
public:
	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
};
