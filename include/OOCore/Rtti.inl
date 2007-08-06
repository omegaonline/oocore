#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

template <class I>
void Omega::System::MetaInfo::iface_stub_functor<I>::init(typename interface_info<I>::safe_class* pS, const guid_t& iid)
{
	if (pS)
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = pS->QueryInterface_Safe(&pObjS,OMEGA_UUIDOF(IObject));
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);

		auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
		m_pI = static_cast<I*>(lookup_proxy(pObjS,iid,false));
		ptrObjS.detach();
	}
}

template <class I>
void Omega::System::MetaInfo::iface_stub_functor<I>::detach(typename interface_info<I>::safe_class*& result, const guid_t& iid)
{
	if (result)
		result->Release_Safe();

	result = 0;
	if (m_pI)
	{
		auto_iface_ptr<IObject> ptrObj(m_pI->QueryInterface(OMEGA_UUIDOF(IObject)));
		if (!ptrObj)
			throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);

		result = static_cast<typename interface_info<I>::safe_class*>(lookup_stub(ptrObj,iid));
		ptrObj.detach();
	}
}

template <class I>
void Omega::System::MetaInfo::iface_proxy_functor<I>::init(I* pI, const guid_t& iid)
{
	m_iid = iid;

	if (pI)
	{
		auto_iface_ptr<IObject> ptrObj(pI->QueryInterface(OMEGA_UUIDOF(IObject)));
		if (!ptrObj)
			throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);

		m_pS = static_cast<typename interface_info<I*>::safe_class>(lookup_stub(ptrObj,m_iid));
	}
}

template <class I>
void Omega::System::MetaInfo::iface_proxy_functor<I>::detach(I* volatile & result)
{
	if (result)
		result->Release();

	result = 0;
	if (m_pS)
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = m_pS->QueryInterface_Safe(&pObjS,OMEGA_UUIDOF(IObject));
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			throw INoInterfaceException::Create(OMEGA_UUIDOF(IObject),OMEGA_SOURCE_INFO);

		auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
		result = static_cast<I*>(lookup_proxy(pObjS,m_iid,true));
		ptrObjS.detach();
	}
}

template <class I>
Omega::System::MetaInfo::iface_stub_functor_array<I>::~iface_stub_functor_array()
{
	if (m_cbSize > m_alloc_size)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;

		OMEGA_THROW(L"Array has been resized out of bounds");
	}

	if (m_piids)
	{
		for (uint32_t i=0;i<m_cbSize;++i)
			m_pFunctors[i].detach(m_pResults[i],m_piids[i]);
	}
	else
	{
		for (uint32_t i=0;i<m_cbSize;++i)
			m_pFunctors[i].detach(m_pResults[i],m_iid);
	}

	delete [] m_pFunctors;
	delete [] m_pVals;
}

template <class I>
void Omega::System::MetaInfo::iface_stub_functor_array<I>::init(typename interface_info<I>::safe_class* pVals)
{
	try
	{
		if (m_cbSize>0)
		{
			OMEGA_NEW(m_pFunctors,typename interface_info<I>::stub_functor[m_cbSize]);
			OMEGA_NEW(m_pVals,I[m_cbSize]);

			if (m_piids)
			{
				for (uint32_t i=0;i<m_cbSize;++i)
					m_pFunctors[i].attach(m_pVals[i],pVals[i],m_piids[i]);
			}
			else
			{
				for (uint32_t i=0;i<m_cbSize;++i)
					m_pFunctors[i].attach(m_pVals[i],pVals[i],m_iid);
			}
		}
	}
	catch (...)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;
		throw;
	}
}

template <class I>
Omega::System::MetaInfo::iface_proxy_functor_array<I>::~iface_proxy_functor_array()
{
	if (m_cbSize > m_alloc_size)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;

		OMEGA_THROW(L"Array has been resized out of bounds");
	}

	if (m_piids)
	{
		for (uint32_t i=0;i<m_cbSize;++i)
			m_pFunctors[i].detach(m_pResults[i],m_piids[i]);
	}
	else
	{
		for (uint32_t i=0;i<m_cbSize;++i)
			m_pFunctors[i].detach(m_pResults[i],m_iid);
	}

	delete [] m_pFunctors;
	delete [] m_pVals;
}

template <class I>
void Omega::System::MetaInfo::iface_proxy_functor_array<I>::init(I* pVals)
{
	try
	{
		if (m_cbSize>0)
		{
			OMEGA_NEW(m_pFunctors,typename interface_info<I>::proxy_functor[m_cbSize]);
			OMEGA_NEW(m_pVals,typename interface_info<I>::safe_class[m_cbSize]);

			if (m_piids)
			{
				for (uint32_t i=0;i<m_cbSize;++i)
					m_pFunctors[i].attach(m_pVals[i],pVals[i],m_piids[i]);
			}
			else
			{
				for (uint32_t i=0;i<m_cbSize;++i)
					m_pFunctors[i].attach(m_pVals[i],pVals[i],m_iid);
			}
		}
	}
	catch (...)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;
		throw;
	}
}

Omega::System::MetaInfo::IException_Safe* OMEGA_CALL Omega::System::MetaInfo::SafeStub::QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid)
{
	if (iid==OMEGA_UUIDOF(IObject))
	{
		*retval = this;
		(*retval)->AddRef_Safe();
		return 0;
	}

	*retval = 0;
	try
	{
		try
		{
			IObject_Safe* pObjS = 0;
			IObject_Safe* pQI = 0;
			
			// See if we have it already, or can QI for it...
			{
				System::ReadGuard guard(m_lock);

				std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.find(iid);
				if (i != m_iid_map.end())
				{
					if (i->second)
						return i->second->QueryInterface_Safe(retval,iid);
					else
						return 0;
				}
				
				// QI all entries
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second)
					{
						IException_Safe* pSE = i->second->QueryInterface_Safe(&pQI,iid);
						if (pSE)
							return pSE;
					}
                    if (pQI)
					{
						pObjS = i->second;
						break;
					}
				}
			}

			if (!pObjS)
			{
				// New stub required
				const qi_rtti* pRtti = get_qi_rtti_info(iid);
				if (!pRtti || !pRtti->pfnCreateSafeStub)
					throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

				pObjS = pRtti->pfnCreateSafeStub(this,m_pObj);
			}

			System::WriteGuard guard(m_lock);
            
			auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
			std::pair<std::map<const guid_t,IObject_Safe*>::iterator,bool> p = m_iid_map.insert(std::map<const guid_t,IObject_Safe*>::value_type(iid,pObjS));
			if (p.second)
				ptrObjS.detach();
			else
				pObjS = p.first->second;

			if (pQI)
			{
				*retval = pQI;
				(*retval)->AddRef_Safe();
				return 0;
			}
			
			if (pObjS)
				return pObjS->QueryInterface_Safe(retval,iid);
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e.what());
		}
	}
	catch (IException* pE)
	{
		return return_safe_exception(pE);
	}

	return 0;
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
		IObject* pObj = 0;
		IObject* pQI = 0;

		// See if we have it already or can QI for it...
		{
			System::ReadGuard guard(m_lock);

			std::map<const guid_t,IObject*>::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
			{
				if (i->second)
					return i->second->QueryInterface(iid);
				else
					return 0;
			}
			
			// QI all entries
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (i->second)
					pQI = i->second->QueryInterface(iid);

				if (pQI)
				{
					pObj = i->second;
					break;
				}
			}
		}

		if (!pObj)
		{
			// New interface required
			const qi_rtti* pRtti = get_qi_rtti_info(iid);
			if (pRtti)
			{
				if (!pRtti->pfnCreateSafeProxy)
					throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

				pObj = pRtti->pfnCreateSafeProxy(this,m_pS);
			}
		}

		System::WriteGuard guard(m_lock);

		auto_iface_ptr<IObject> ptrObj(pObj);
		std::pair<std::map<const guid_t,IObject*>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,IObject*>::value_type(iid,pObj));
		if (p.second)
			ptrObj.detach();
		else
			pObj = p.first->second;
		
		if (pQI)
		{
			pQI->AddRef();
			return pQI;
		}

		if (pObj)
			return pObj->QueryInterface(iid);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return 0;
}

Omega::System::MetaInfo::SafeProxyStubMap& Omega::System::MetaInfo::get_proxy_map()
{
	static SafeProxyStubMap proxy_map;
	return proxy_map;
}

Omega::System::MetaInfo::SafeProxyStubMap& Omega::System::MetaInfo::get_stub_map()
{
	static SafeProxyStubMap stub_map;
	return stub_map;
}

Omega::System::MetaInfo::IObject_Safe* Omega::System::MetaInfo::lookup_stub(Omega::IObject* pObj, const Omega::guid_t& iid)
{
	SafeProxyStubMap& stub_map = get_stub_map();
	
	auto_iface_safe_ptr<IObject_Safe> ptrSafeStub;
	try
	{
		// Lookup first
		{
			System::ReadGuard guard(stub_map.m_lock);

			std::map<void*,void*>::iterator i=stub_map.m_map.find(pObj);
			if (i != stub_map.m_map.end())
			{
				ptrSafeStub = static_cast<IObject_Safe*>(i->second);
			}
		}

		if (!ptrSafeStub)
		{
			auto_iface_ptr<SafeProxy> ptrSafeProxy(static_cast<SafeProxy*>(pObj->QueryInterface(OMEGA_UUIDOF(SafeProxy))));
			if (ptrSafeProxy)
			{
				ptrSafeStub = ptrSafeProxy->GetSafeStub();
			}
			else
			{
                OMEGA_NEW(ptrSafeStub,SafeStub(pObj));
			
				System::WriteGuard guard(stub_map.m_lock);

				std::pair<std::map<void*,void*>::iterator,bool> p = stub_map.m_map.insert(std::map<void*,void*>::value_type(pObj,static_cast<IObject_Safe*>(ptrSafeStub)));
				if (!p.second)
					ptrSafeStub = static_cast<IObject_Safe*>(p.first->second);
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	IObject_Safe* pRet = 0;
	IException_Safe* pSE = ptrSafeStub->QueryInterface_Safe(&pRet,iid);
	if (pSE)
		throw_correct_exception(pSE);

	if (!pRet)
		throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
	
	return pRet;
}

Omega::IObject* Omega::System::MetaInfo::lookup_proxy(Omega::System::MetaInfo::IObject_Safe* pObjS, const Omega::guid_t& iid, bool bPartialAllowed)
{
	SafeProxyStubMap& proxy_map = get_proxy_map();
	
	auto_iface_ptr<IObject> ptrSafeProxy;
	try
	{
		// Lookup first
		{
			System::ReadGuard guard(proxy_map.m_lock);

			std::map<void*,void*>::iterator i=proxy_map.m_map.find(pObjS);
			if (i != proxy_map.m_map.end())
			{
				ptrSafeProxy = static_cast<IObject*>(i->second);
			}
		}

		if (!ptrSafeProxy)
		{
			OMEGA_NEW(ptrSafeProxy,SafeProxy(pObjS));

			System::WriteGuard guard(proxy_map.m_lock);

			std::pair<std::map<void*,void*>::iterator,bool> p = proxy_map.m_map.insert(std::map<void*,void*>::value_type(pObjS,static_cast<IObject*>(ptrSafeProxy)));
			if (!p.second)
				ptrSafeProxy = static_cast<IObject*>(p.first->second);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	IObject* pRet = ptrSafeProxy->QueryInterface(iid);
	if (!pRet)
	{
		if (bPartialAllowed)
		{
			pRet = ptrSafeProxy;
			ptrSafeProxy.detach();
		}
		else
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
	}

	return pRet;
}

Omega::System::MetaInfo::IException_Safe* Omega::System::MetaInfo::return_safe_exception(Omega::IException* pE)
{
	guid_t iid = pE->ActualIID();

	// Wrap with the correct _SafeStub wrapper by calling QI
	auto_iface_ptr<IException> ptrE(pE);
	IObject_Safe* pSE2 = 0;
	IException_Safe* pSE3 = static_cast<IException_Safe*>(interface_info<IException*>::proxy_functor(pE))->QueryInterface_Safe(&pSE2,iid);
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

