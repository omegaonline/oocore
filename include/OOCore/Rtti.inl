///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

Omega::System::MetaInfo::IException_Safe* OMEGA_CALL Omega::System::MetaInfo::SafeStub::QueryInterface_Safe(const guid_t* piid, IObject_Safe** retval)
{
	if (*piid==OMEGA_UUIDOF(IObject))
	{
		*retval = this;
		(*retval)->AddRef_Safe();
		return 0;
	}

	*retval = 0;

	try
	{
		auto_iface_safe_ptr<IObject_Safe> ptrQI;
		auto_iface_safe_ptr<IObject_Safe> ptrStub;

		// See if we have it already
		{
			Threading::ReadGuard guard(m_lock);

			std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.find(*piid);
			if (i != m_iid_map.end())
			{
				IException_Safe* pSE = i->second->QueryInterface_Safe(piid,retval);
				if (pSE)
					return pSE;
				return 0;
			}

			// See if any known interface supports the new interface
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				IObject_Safe* pQI = 0;
				IException_Safe* pSE = i->second->QueryInterface_Safe(piid,&pQI);
				if (pSE)
					return pSE;
				if (pQI)
				{
					ptrQI.attach(pQI);
					ptrStub = i->second;
					break;
				}
			}
		}

		if (!ptrQI)
		{
			// Check whether underlying object supports interface
			auto_iface_ptr<IObject> ptrI(m_pObj->QueryInterface(*piid));
			if (!ptrI)
				return 0;

			// New stub required
			const qi_rtti* pRtti = get_qi_rtti_info(*piid);
			if (!pRtti || !pRtti->pfnCreateSafeStub)
				throw INoInterfaceException::Create(*piid,OMEGA_SOURCE_INFO);

			ptrStub = pRtti->pfnCreateSafeStub(this,ptrI);
		}

		{
			Threading::WriteGuard guard(m_lock);

			std::pair<std::map<const guid_t,IObject_Safe*>::iterator,bool> p = m_iid_map.insert(std::map<const guid_t,IObject_Safe*>::value_type(*piid,ptrStub));
			if (!p.second)
			{
				ptrQI = 0;
				ptrStub = p.first->second;
			}
			else
			{
				ptrStub->AddRef_Safe();
			}
		}

		if (!ptrQI)
		{
			IObject_Safe* pQI = 0;
			IException_Safe* pSE = ptrStub->QueryInterface_Safe(piid,&pQI);
			if (pSE)
				return pSE;
			ptrQI.attach(pQI);
		}

		*retval = ptrQI;
		ptrQI.detach();
		return 0;
	}
	catch (std::exception& e)
	{
		return return_safe_exception(Omega::ISystemException::Create(e,OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return return_safe_exception(pE);
	}
}

Omega::IObject* Omega::System::MetaInfo::SafeProxy::ProxyQI(const guid_t& iid, bool bPartialAllowed)
{
	if (iid==OMEGA_UUIDOF(IObject) ||
		iid==OMEGA_UUIDOF(ISafeProxy))
	{
		AddRef();
		return this;
	}

	try
	{
		auto_iface_ptr<IObject> ptrQI;
		auto_iface_ptr<IObject> ptrProxy;

		// See if we have it already...
		{
			Threading::ReadGuard guard(m_lock);

			std::map<const guid_t,IObject*>::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
				return i->second->QueryInterface(iid);

			// See if any known interface supports the new interface
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				ptrQI.attach(i->second->QueryInterface(iid));
				if (ptrQI)
				{
					ptrProxy = i->second;
					break;
				}
			}
		}

		if (!ptrProxy)
		{
			// Check whether underlying object supports interface
			IObject_Safe* pS = 0;
			IException_Safe* pSE = m_pS->QueryInterface_Safe(&iid,&pS);
			if (pSE)
				throw_correct_exception(pSE);
			if (!pS)
				return 0;

			auto_iface_safe_ptr<IObject_Safe> ptrS(pS);

			// New interface required
			const qi_rtti* pRtti = get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateSafeProxy)
			{
				if (!bPartialAllowed)
					throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
				else
				{
					AddRef();
					return this;
				}
			}

			ptrProxy = pRtti->pfnCreateSafeProxy(this,ptrS);
		}

		{
			Threading::WriteGuard guard(m_lock);

			std::pair<std::map<const guid_t,IObject*>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,IObject*>::value_type(iid,ptrProxy));
			if (!p.second)
			{
				ptrQI = 0;
				ptrProxy = p.first->second;
			}
			else
			{
				ptrProxy->AddRef();
			}
		}

		if (!ptrQI)
			ptrQI.attach(ptrProxy->QueryInterface(iid));

		IObject* pRet = ptrQI;
		ptrQI.detach();
		return pRet;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::SafeStubMap& Omega::System::MetaInfo::get_stub_map()
{
	static SafeStubMap stub_map;
	return stub_map;
}

template <class I>
typename Omega::System::MetaInfo::interface_info<I>::safe_class* Omega::System::MetaInfo::lookup_stub(I* pI, const guid_t& iid)
{
	if (!pI)
		return 0;

	SafeStubMap& stub_map = get_stub_map();

	auto_iface_ptr<IObject> ptrObj(pI->QueryInterface(OMEGA_UUIDOF(IObject)));
	if (!ptrObj)
		throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);

	auto_iface_safe_ptr<IObject_Safe> ptrStub;
	try
	{
		// Lookup first
		{
			Threading::ReadGuard guard(stub_map.m_lock);

			std::map<IObject*,IObject_Safe*>::iterator i=stub_map.m_map.find(ptrObj);
			if (i != stub_map.m_map.end())
			{
				ptrStub = i->second;
			}
		}

		if (!ptrStub)
		{
			auto_iface_ptr<ISafeProxy> ptrProxy(static_cast<ISafeProxy*>(ptrObj->QueryInterface(OMEGA_UUIDOF(ISafeProxy))));
			if (ptrProxy)
			{
				ptrStub = ptrProxy->GetSafeStub();
			}
			else
			{
				OMEGA_NEW(ptrStub,SafeStub(ptrObj));

				Threading::WriteGuard guard(stub_map.m_lock);

				std::pair<std::map<IObject*,IObject_Safe*>::iterator,bool> p = stub_map.m_map.insert(std::map<IObject*,IObject_Safe*>::value_type(ptrObj,ptrStub));
				if (!p.second)
				{
					auto_iface_safe_ptr<IObject_Safe> p2 = p.first->second;

					guard.Unlock();

					ptrStub = static_cast<IObject_Safe*>(p2);
				}
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	IObject_Safe* pRet = 0;
	IException_Safe* pSE = ptrStub->QueryInterface_Safe(&iid,&pRet);
	if (pSE)
		throw_correct_exception(pSE);

	if (!pRet)
		throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

	return static_cast<typename interface_info<I>::safe_class*>(pRet);
}

Omega::System::MetaInfo::SafeProxyMap& Omega::System::MetaInfo::get_proxy_map()
{
	static SafeProxyMap proxy_map;
	return proxy_map;
}

template <class I>
I* Omega::System::MetaInfo::lookup_proxy(typename interface_info<I>::safe_class* pS, const guid_t& iid, bool bPartialAllowed)
{
	if (!pS)
		return 0;

	SafeProxyMap& proxy_map = get_proxy_map();

	IObject_Safe* pObjS = 0;
	IException_Safe* pSE = pS->QueryInterface_Safe(&OMEGA_UUIDOF(IObject),&pObjS);
	if (pSE)
		throw_correct_exception(pSE);
	if (!pObjS)
		throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);
	auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

	auto_iface_ptr<ISafeProxy> ptrProxy;
	try
	{
		// Lookup first
		{
			Threading::ReadGuard guard(proxy_map.m_lock);

			std::map<IObject_Safe*,ISafeProxy*>::iterator i=proxy_map.m_map.find(ptrObjS);
			if (i != proxy_map.m_map.end())
			{
				ptrProxy = i->second;
			}
		}

		if (!ptrProxy)
		{
			OMEGA_NEW(ptrProxy,SafeProxy(pObjS));

			Threading::WriteGuard guard(proxy_map.m_lock);

			std::pair<std::map<IObject_Safe*,ISafeProxy*>::iterator,bool> p = proxy_map.m_map.insert(std::map<IObject_Safe*,ISafeProxy*>::value_type(pObjS,ptrProxy));
			if (!p.second)
			{
				auto_iface_ptr<ISafeProxy> p2 = p.first->second;

				guard.Unlock();

				ptrProxy = static_cast<ISafeProxy*>(p2);
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	I* pRet = static_cast<I*>(ptrProxy->ProxyQI(iid,bPartialAllowed));
	if (!pRet)
		throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

	return pRet;
}

Omega::System::MetaInfo::IException_Safe* Omega::System::MetaInfo::return_safe_exception(IException* pE)
{
	guid_t iid = pE->ActualIID();

	// Wrap with the correct _SafeStub wrapper by calling QI
	auto_iface_ptr<IException> ptrE(pE);
	IObject_Safe* pSE2 = 0;
	IException_Safe* pSE3 = marshal_info<IException*>::safe_type::coerce(pE)->QueryInterface_Safe(&iid,&pSE2);
	if (pSE3)
		return pSE3;
	if (!pSE2)
		return return_safe_exception(INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO));

	return static_cast<IException_Safe*>(pSE2);
}

void Omega::System::MetaInfo::throw_correct_exception(IException_Safe* pSE)
{
	guid_t iid;
	IException_Safe* pSE2 = pSE->ActualIID_Safe(&iid);
	if (pSE2)
		throw_correct_exception(pSE2);
	else
	{
		const qi_rtti* pRtti = get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnSafeThrow)
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

		pRtti->pfnSafeThrow(pSE);
	}
}

Omega::string_t Omega::System::MetaInfo::lookup_iid(const guid_t& iid)
{
	static const string_t strUnk = L"Unknown";
	const qi_rtti* pRtti = get_qi_rtti_info(iid);
	if (!pRtti)
		return strUnk;

	return pRtti->strName;
}

bool Omega::System::PinObjectPointer(IObject* pObject)
{
	if (pObject)
	{
		Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_UUIDOF(Omega::System::MetaInfo::ISafeProxy))));
		if (ptrProxy)
		{
			ptrProxy->Pin();
			return true;
		}
	}

	return false;
}

void Omega::System::UnpinObjectPointer(IObject* pObject)
{
	Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_UUIDOF(Omega::System::MetaInfo::ISafeProxy))));
	if (ptrProxy)
		ptrProxy->Unpin();
}

OMEGA_EXPORTED_FUNCTION(Omega::ISystemException*,ISystemException_Create_errno,2,((in),Omega::uint32_t,e,(in),const Omega::string_t&,source))
Omega::ISystemException* Omega::ISystemException::Create(Omega::uint32_t errno_val, const Omega::string_t& source)
{
	return ISystemException_Create_errno(errno_val,source);
}

OMEGA_EXPORTED_FUNCTION(Omega::ISystemException*,ISystemException_Create,2,((in),const Omega::string_t&,desc,(in),const Omega::string_t&,source));
Omega::ISystemException* Omega::ISystemException::Create(const std::exception& e, const Omega::string_t& source)
{
	return ISystemException_Create(string_t(e.what(),false),source);
}

Omega::ISystemException* Omega::ISystemException::Create(const string_t& desc, const Omega::string_t& source)
{
	return ISystemException_Create(desc,source);
}

OMEGA_EXPORTED_FUNCTION(Omega::INoInterfaceException*,INoInterfaceException_Create,2,((in),const Omega::guid_t&,iid,(in),const Omega::string_t&,source));
Omega::INoInterfaceException* Omega::INoInterfaceException::Create(const Omega::guid_t& iid, const string_t& source)
{
	return INoInterfaceException_Create(iid,source);
}

#endif // OOCORE_RTTI_INL_INCLUDED_

