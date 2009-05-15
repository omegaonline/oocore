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

template <class K, class V>
bool Omega::System::MetaInfo::PSMap<K,V>::load(K key, V& value)
{
	try
	{
		Threading::ReadGuard<Threading::ReaderWriterLock> guard(m_lock);

		typename map_type::const_iterator i = m_map.find(key);
		if (i != m_map.end())
		{
			value = i->second;
			return true;
		}
		
		return false;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

template <class K, class V>
bool Omega::System::MetaInfo::PSMap<K,V>::store(K key, V value)
{
	try
	{
		Threading::Guard<Threading::ReaderWriterLock> guard(m_lock);

		std::pair<typename map_type::iterator,bool> p = m_map.insert(typename map_type::value_type(key,value));
		return p.second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

template <class K, class V>
void Omega::System::MetaInfo::PSMap<K,V>::erase(K key)
{
	try
	{
		Threading::Guard<Threading::ReaderWriterLock> guard(m_lock);

		m_map.erase(key);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

Omega::System::MetaInfo::IException_Safe* OMEGA_CALL Omega::System::MetaInfo::SafeStub::QueryInterface_Safe(const guid_t* piid, IObject_Safe** retval)
{
	if (*piid==OMEGA_GUIDOF(IObject))
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
			Threading::ReadGuard<Threading::ReaderWriterLock> guard(m_lock);

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
			Threading::Guard<Threading::ReaderWriterLock> guard(m_lock);

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
	if (iid==OMEGA_GUIDOF(IObject) ||
		iid==OMEGA_GUIDOF(ISafeProxy))
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
			Threading::ReadGuard<Threading::ReaderWriterLock> guard(m_lock);

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
			Threading::Guard<Threading::ReaderWriterLock> guard(m_lock);

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

template <class I>
typename Omega::System::MetaInfo::interface_info<I>::safe_class* Omega::System::MetaInfo::lookup_stub(I* pI, const guid_t& iid)
{
	if (!pI)
		return 0;

	auto_iface_ptr<IObject> ptrObj(pI->QueryInterface(OMEGA_GUIDOF(IObject)));
	if (!ptrObj)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(IObject),OMEGA_SOURCE_INFO);

	auto_iface_safe_ptr<IObject_Safe> ptrStub;
	try
	{
		for (;;)
		{
			// Lookup first
			if (stub_map_load(ptrObj,ptrStub))
				break;
			
			auto_iface_ptr<ISafeProxy> ptrProxy(static_cast<ISafeProxy*>(ptrObj->QueryInterface(OMEGA_GUIDOF(ISafeProxy))));
			if (ptrProxy)
			{
				ptrStub = ptrProxy->GetSafeStub();
				break;
			}
			
			OMEGA_NEW(ptrStub,SafeStub(ptrObj));

			if (stub_map_store(ptrObj,ptrStub))
				break;

			// One has been created while we locked - loop
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
		throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

	return static_cast<typename interface_info<I>::safe_class*>(pRet);
}

template <class I>
I* Omega::System::MetaInfo::lookup_proxy(typename interface_info<I>::safe_class* pS, const guid_t& iid, bool bPartialAllowed)
{
	if (!pS)
		return 0;

	IObject_Safe* pObjS = 0;
	IException_Safe* pSE = pS->QueryInterface_Safe(&OMEGA_GUIDOF(IObject),&pObjS);
	if (pSE)
		throw_correct_exception(pSE);
	if (!pObjS)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(IObject),OMEGA_SOURCE_INFO);
	auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

	auto_iface_ptr<ISafeProxy> ptrProxy;
	try
	{
		for (;;)
		{
			// Lookup first
			if (proxy_map_load(ptrObjS,ptrProxy))
				break;
			
			OMEGA_NEW(ptrProxy,SafeProxy(pObjS));

			if (proxy_map_store(pObjS,ptrProxy))
				break;

			// One has been created while we locked - loop
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
	guid_t iid = pE->GetThrownIID();

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
	IException_Safe* pSE2 = pSE->GetThrownIID_Safe(&iid);
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

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoTypeInfo,3,((in),const Omega::guid_t&,iid,(in),const wchar_t*,pszName,(in),const void*,type_info));
void Omega::System::MetaInfo::RegisterAutoTypeInfo(const guid_t& iid, const wchar_t* pszName, const typeinfo_rtti* type_info)
{
	OOCore_RegisterAutoTypeInfo(iid,pszName,(const void*)type_info);
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_UnregisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),const void*,type_info));
void Omega::System::MetaInfo::UnregisterAutoTypeInfo(const guid_t& iid, const typeinfo_rtti* type_info)
{
	OOCore_UnregisterAutoTypeInfo(iid,(const void*)type_info);
}

bool Omega::System::PinObjectPointer(IObject* pObject)
{
	if (pObject)
	{
		Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::MetaInfo::ISafeProxy))));
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
	Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::ISafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::ISafeProxy*>(pObject->QueryInterface(OMEGA_GUIDOF(Omega::System::MetaInfo::ISafeProxy))));
	if (ptrProxy)
		ptrProxy->Unpin();
}

OMEGA_EXPORTED_FUNCTION(Omega::ISystemException*,OOCore_ISystemException_Create_errno,2,((in),Omega::uint32_t,e,(in),const Omega::string_t&,source))
Omega::ISystemException* Omega::ISystemException::Create(uint32_t errno_val, const string_t& source)
{
	return OOCore_ISystemException_Create_errno(errno_val,source);
}

OMEGA_EXPORTED_FUNCTION(Omega::ISystemException*,OOCore_ISystemException_Create,2,((in),const Omega::string_t&,desc,(in),const Omega::string_t&,source))
Omega::ISystemException* Omega::ISystemException::Create(const std::exception& e, const string_t& source)
{
	return OOCore_ISystemException_Create(string_t(e.what(),false),source);
}

Omega::ISystemException* Omega::ISystemException::Create(const string_t& desc, const string_t& source)
{
	return OOCore_ISystemException_Create(desc,source);
}

OMEGA_EXPORTED_FUNCTION(Omega::INoInterfaceException*,OOCore_INoInterfaceException_Create,2,((in),const Omega::guid_t&,iid,(in),const Omega::string_t&,source))
Omega::INoInterfaceException* Omega::INoInterfaceException::Create(const guid_t& iid, const string_t& source)
{
	return OOCore_INoInterfaceException_Create(iid,source);
}

OMEGA_EXPORTED_FUNCTION(Omega::ITimeoutException*,OOCore_ITimeoutException_Create,0,())
Omega::ITimeoutException* Omega::ITimeoutException::Create()
{
	return OOCore_ITimeoutException_Create();
}

#endif // OOCORE_RTTI_INL_INCLUDED_

