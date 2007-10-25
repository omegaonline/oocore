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
			System::ReadGuard guard(m_lock);

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
			System::WriteGuard guard(m_lock);

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
		return return_safe_exception(Omega::IException::Create(string_t(e.what(),false),OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return return_safe_exception(pE);
	}
}

Omega::IObject* Omega::System::MetaInfo::SafeProxy::QueryInterface(const guid_t& iid)
{
	if (iid==OMEGA_UUIDOF(IObject) ||
		iid==OMEGA_UUIDOF(SafeProxy))
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
			System::ReadGuard guard(m_lock);

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
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			ptrProxy = pRtti->pfnCreateSafeProxy(this,ptrS);
		}
		
		{
			System::WriteGuard guard(m_lock);
				
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
		OMEGA_THROW(string_t(e.what(),false));
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
			System::ReadGuard guard(stub_map.m_lock);

			std::map<IObject*,IObject_Safe*>::iterator i=stub_map.m_map.find(ptrObj);
			if (i != stub_map.m_map.end())
			{
				ptrStub = i->second;
			}
		}

		if (!ptrStub)
		{
			auto_iface_ptr<SafeProxy> ptrProxy(static_cast<SafeProxy*>(ptrObj->QueryInterface(OMEGA_UUIDOF(SafeProxy))));
			if (ptrProxy)
			{
				ptrStub = ptrProxy->GetSafeStub();
			}
			else
			{
                OMEGA_NEW(ptrStub,SafeStub(ptrObj));

				System::WriteGuard guard(stub_map.m_lock);

				std::pair<std::map<IObject*,IObject_Safe*>::iterator,bool> p = stub_map.m_map.insert(std::map<IObject*,IObject_Safe*>::value_type(ptrObj,ptrStub));
				if (!p.second)
					ptrStub = p.first->second;
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
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

	auto_iface_ptr<IObject> ptrProxy;
	try
	{
		// Lookup first
		{
			System::ReadGuard guard(proxy_map.m_lock);

			std::map<IObject_Safe*,IObject*>::iterator i=proxy_map.m_map.find(ptrObjS);
			if (i != proxy_map.m_map.end())
			{
				ptrProxy = i->second;
			}
		}

		if (!ptrProxy)
		{
			OMEGA_NEW(ptrProxy,SafeProxy(pObjS));
			
			System::WriteGuard guard(proxy_map.m_lock);

			std::pair<std::map<IObject_Safe*,IObject*>::iterator,bool> p = proxy_map.m_map.insert(std::map<IObject_Safe*,IObject*>::value_type(pObjS,ptrProxy));
			if (!p.second)
				ptrProxy = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	I* pRet = static_cast<I*>(ptrProxy->QueryInterface(iid));
	if (!pRet)
	{
		if (bPartialAllowed)
		{
			pRet = static_cast<I*>(static_cast<IObject*>(ptrProxy));
			pRet->AddRef();
		}
		else
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
	}
		
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

const Omega::string_t& Omega::System::MetaInfo::lookup_iid(const guid_t& iid)
{
	static const string_t strUnk = L"Unknown";
	const qi_rtti* pRtti = get_qi_rtti_info(iid);
	if (!pRtti)
		return strUnk;

	return pRtti->strName;
}

void Omega::PinObjectPointer(IObject* pObject)
{
	Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::SafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::SafeProxy*>(pObject->QueryInterface(OMEGA_UUIDOF(Omega::System::MetaInfo::SafeProxy))));
	if (ptrProxy)
		ptrProxy->Pin();	
}

void Omega::UnpinObjectPointer(IObject* pObject)
{
	Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::SafeProxy> ptrProxy(static_cast<Omega::System::MetaInfo::SafeProxy*>(pObject->QueryInterface(OMEGA_UUIDOF(Omega::System::MetaInfo::SafeProxy))));
	if (ptrProxy)
		ptrProxy->Unpin();
}

OOCORE_EXPORTED_FUNCTION(Omega::IException*,IException_Create,3,((in),const Omega::string_t&,desc,(in),const Omega::string_t&,source,(in),Omega::IException*,pCause));
Omega::IException* Omega::IException::Create(const Omega::string_t& desc, const Omega::string_t& source, Omega::IException* pCause)
{
	return IException_Create(desc,source,pCause);
}

OOCORE_EXPORTED_FUNCTION(Omega::INoInterfaceException*,INoInterfaceException_Create,2,((in),const Omega::guid_t&,iid,(in),const Omega::string_t&,source));
Omega::INoInterfaceException* Omega::INoInterfaceException::Create(const Omega::guid_t& iid, const string_t& source)
{
	return INoInterfaceException_Create(iid,source);
}

#endif // OOCORE_RTTI_INL_INCLUDED_

