#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

template <class I>
void Omega::MetaInfo::iface_stub_functor<I>::init(typename interface_info<I>::safe_class* pS, const guid_t& iid)
{
	if (pS)
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = pS->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_SOURCE_INFO);

		auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
		m_pI = static_cast<I*>(lookup_proxy(pObjS,iid,false));
		ptrObjS.detach();
	}
}

template <class I>
void Omega::MetaInfo::iface_stub_functor<I>::detach(typename interface_info<I>::safe_class*& result, const guid_t& iid)
{
	if (result)
		result->Release_Safe();

	result = 0;
	if (m_pI)
	{
		auto_iface_ptr<IObject> ptrObj(m_pI->QueryInterface(IID_IObject));
		if (!ptrObj)
			INoInterfaceException::Throw(IID_IObject,OMEGA_SOURCE_INFO);

		result = static_cast<typename interface_info<I>::safe_class*>(lookup_stub(ptrObj,iid));
		ptrObj.detach();
	}
}

template <class I>
void Omega::MetaInfo::iface_proxy_functor<I>::init(I* pI, const guid_t& iid)
{
	m_iid = iid;

	if (pI)
	{
		auto_iface_ptr<IObject> ptrObj(pI->QueryInterface(IID_IObject));
		if (!ptrObj)
			INoInterfaceException::Throw(IID_IObject,OMEGA_SOURCE_INFO);

		m_pS = static_cast<typename interface_info<I*>::safe_class>(lookup_stub(ptrObj,m_iid));
	}
}

template <class I>
void Omega::MetaInfo::iface_proxy_functor<I>::detach(I* volatile & result)
{
	if (result)
		result->Release();

	result = 0;
	if (m_pS)
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = m_pS->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_SOURCE_INFO);

		auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
		result = static_cast<I*>(lookup_proxy(pObjS,m_iid,true));
		ptrObjS.detach();
	}
}

template <class I>
Omega::MetaInfo::iface_stub_functor_array<I>::~iface_stub_functor_array()
{
	if (m_cbSize > m_alloc_size)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;

		OMEGA_THROW("Array has been resized out of bounds");
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
void Omega::MetaInfo::iface_stub_functor_array<I>::init(typename interface_info<I>::safe_class* pVals)
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
Omega::MetaInfo::iface_proxy_functor_array<I>::~iface_proxy_functor_array()
{
	if (m_cbSize > m_alloc_size)
	{
		delete [] m_pFunctors;
		delete [] m_pVals;

		OMEGA_THROW("Array has been resized out of bounds");
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
void Omega::MetaInfo::iface_proxy_functor_array<I>::init(I* pVals)
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

Omega::MetaInfo::IException_Safe* OMEGA_CALL Omega::MetaInfo::SafeStub::QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid)
{
	if (iid==IID_IObject)
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
			bool bNew = false;
			IObject_Safe* pObjS = 0;

			// See if we have it already, or can QI for it...
			{
				ReadGuard guard(m_lock);

				std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.find(iid);
				if (i == m_iid_map.end())
				{
					// QI all entries
					for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
					{
						IException_Safe* pSE = i->second->QueryInterface_Safe(&pObjS,iid);
						if (pSE)
							return pSE;
						if (pObjS)
						{
							bNew = true;
							break;
						}
					}
				}
			}

			if (!pObjS)
			{
				// New stub required
				const qi_rtti* pRtti = get_qi_rtti_info(iid);
				if (!pRtti || !pRtti->pfnCreateSafeStub)
					INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

				pObjS = pRtti->pfnCreateSafeStub(this,m_pObj);
				bNew = true;
			}

			if (bNew)
			{
				WriteGuard guard(m_lock);
                
				auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);
				m_iid_map.insert(std::map<const guid_t,IObject_Safe*>::value_type(iid,pObjS));
				ptrObjS.detach();
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

Omega::IObject* Omega::MetaInfo::SafeProxy::QueryInterface(const guid_t& iid)
{
	if (iid==IID_IObject ||
		iid==IID_SafeProxy)
	{
		AddRef();
		return this;
	}

	try
	{
		bool bNew = false;
		IObject* pObj = 0;

		// See if we have it already or can QI for it...
		{
			ReadGuard guard(m_lock);

			std::map<const guid_t,IObject*>::iterator i=m_iid_map.find(iid);
			if (i == m_iid_map.end())
			{
				// QI all entries
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					pObj = i->second->QueryInterface(iid);
					if (pObj)
					{
						bNew = true;
						break;
					}
				}
			}
		}

		if (!pObj)
		{
			// New interface required
			const qi_rtti* pRtti = get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateSafeProxy)
				INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

			pObj = pRtti->pfnCreateSafeProxy(this,m_pS);
			bNew = true;
		}

		if (bNew)
		{
			WriteGuard guard(m_lock);

			auto_iface_ptr<IObject> ptrObj(pObj);
			m_iid_map.insert(std::map<const guid_t,IObject*>::value_type(iid,pObj));
			ptrObj.detach();
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

const Omega::MetaInfo::qi_rtti* Omega::MetaInfo::get_qi_rtti_info(const guid_t& iid)
{
	static std::map<const guid_t,const qi_rtti*> mapRtti;
	static ReaderWriterLock rw_lock;

	const qi_rtti* pRet = 0;

	try
	{
		// See if we have it already
		{
			ReadGuard guard(rw_lock);

			std::map<const guid_t,const qi_rtti*>::iterator i=mapRtti.find(iid);
			if (i!=mapRtti.end())
				pRet = i->second;
		}
	
		if (!pRet)
		{
			get_qi_rtti_info_impl<(sizeof(get_qi_rtti(&pRet,(size_t_<0>::type*)0,iid)) == sizeof(yes_t))>::execute(&pRet,(size_t_<0>*)0,iid);

			WriteGuard guard(rw_lock);

			mapRtti.insert(std::map<const guid_t,const qi_rtti*>::value_type(iid,pRet)).first;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return pRet;
};

Omega::MetaInfo::SafeProxyStubMap& Omega::MetaInfo::get_proxy_map()
{
	static SafeProxyStubMap proxy_map;
	return proxy_map;
}

Omega::MetaInfo::SafeProxyStubMap& Omega::MetaInfo::get_stub_map()
{
	static SafeProxyStubMap stub_map;
	return stub_map;
}

Omega::MetaInfo::IObject_Safe* Omega::MetaInfo::lookup_stub(Omega::IObject* pObj, const Omega::guid_t& iid)
{
	SafeProxyStubMap& stub_map = get_stub_map();
	
	auto_iface_safe_ptr<IObject_Safe> ptrSafeStub;
	try
	{
		// Lookup first
		{
			ReadGuard guard(stub_map.m_lock);

			std::map<void*,void*>::iterator i=stub_map.m_map.find(pObj);
			if (i != stub_map.m_map.end())
			{
				ptrSafeStub = static_cast<IObject_Safe*>(i->second);
			}
		}

		if (!ptrSafeStub)
		{
			auto_iface_ptr<SafeProxy> ptrSafeProxy(static_cast<SafeProxy*>(pObj->QueryInterface(IID_SafeProxy)));
			if (ptrSafeProxy)
			{
				ptrSafeStub = ptrSafeProxy->GetSafeStub();
			}
			else
			{
                OMEGA_NEW(ptrSafeStub,SafeStub(pObj));
			}

			WriteGuard guard(stub_map.m_lock);

			stub_map.m_map.insert(std::map<void*,void*>::value_type(pObj,static_cast<IObject_Safe*>(ptrSafeStub)));
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
		Omega::INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

	return pRet;
}

Omega::IObject* Omega::MetaInfo::lookup_proxy(Omega::MetaInfo::IObject_Safe* pObjS, const Omega::guid_t& iid, bool bPartialAllowed)
{
	SafeProxyStubMap& proxy_map = get_proxy_map();
	
	auto_iface_ptr<IObject> ptrSafeProxy;
	try
	{
		// Lookup first
		{
			ReadGuard guard(proxy_map.m_lock);

			std::map<void*,void*>::iterator i=proxy_map.m_map.find(pObjS);
			if (i != proxy_map.m_map.end())
			{
				ptrSafeProxy = static_cast<IObject*>(i->second);
			}
		}

		if (!ptrSafeProxy)
		{
			OMEGA_NEW(ptrSafeProxy,SafeProxy(pObjS));

			WriteGuard guard(proxy_map.m_lock);

			proxy_map.m_map.insert(std::map<void*,void*>::value_type(pObjS,static_cast<IObject*>(ptrSafeProxy)));
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
			INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);
	}

	return pRet;
}

Omega::MetaInfo::IException_Safe* Omega::MetaInfo::return_safe_exception(Omega::IException* pE)
{
	// Wrap with the correct _SafeStub wrapper by calling QI
	auto_iface_ptr<IException> ptrE(pE);
	IObject_Safe* pSE2 = 0;
	IException_Safe* pSE3 = static_cast<IException_Safe*>(interface_info<IException*>::proxy_functor(pE))->QueryInterface_Safe(&pSE2,pE->ActualIID());
	if (pSE3)
		return pSE3;
	if (!pSE2)
	{
		try
		{
			INoInterfaceException::Throw(pE->ActualIID(),OMEGA_SOURCE_INFO);
		}
		catch (IException* pE2)
		{
			return return_safe_exception(pE2);
		}
	}

	return static_cast<IException_Safe*>(pSE2);
}

void Omega::MetaInfo::throw_correct_exception(IException_Safe* pSE)
{
	guid_t iid;
	IException_Safe* pSE2 = pSE->ActualIID_Safe(&iid);
	if (pSE2)
		throw_correct_exception(pSE2);
	else
	{
		const qi_rtti* pRtti = get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnSafeThrow)
			INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

		pRtti->pfnSafeThrow(pSE);
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(IException_Throw,3,((in),const Omega::char_t*,desc,(in),const Omega::char_t*,source,(in),Omega::IException*,pCause));
void Omega::IException::Throw(const Omega::char_t* desc, const Omega::char_t* source, Omega::IException* pCause)
{
	IException_Throw(desc,source,pCause);
}

#endif // OOCORE_RTTI_INL_INCLUDED_

