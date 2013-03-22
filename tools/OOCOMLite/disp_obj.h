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

class IDispatchObjImpl : public IDispatchImpl
{
public:
	IDispatchObjImpl(Omega::Remoting::IProxy* pProxy, const Omega::guid_t& iid);

	HRESULT init();

private:
	virtual ~IDispatchObjImpl();

	Omega::guid_t m_iid;
		
	OTL::ObjectPtr<Omega::Remoting::IProxy>         m_ptrProxy;
	OTL::ObjectPtr<Omega::TypeInfo::IInterfaceInfo> m_ptrInfo;
	OTL::ObjectPtr<Omega::Remoting::IMarshalContext>    m_ptrMarshalContext;

	void ReadArgByType(LCID lcid, const Omega::string_t& strName, Omega::Remoting::IMessage* pMsg, Omega::Remoting::IMessage* pType, VARIANTARG* pArg);
	void ReadArg(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param, VARIANTARG* pArg);
		
	HRESULT WriteArg(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param, const VARIANTARG& arg);
	void WriteDefault(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param);

	void UnpackArgs(LCID lcid, Omega::Remoting::IMessage* pMsg, Omega::uint32_t method, Omega::byte_t param_count);
	
// IDispatch members
public:
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject)
	{
		return IDispatchImpl::QueryInterface(riid,ppvObject);
	}
	
	STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR** rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId);
	STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
};
