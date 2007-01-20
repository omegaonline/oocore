#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

template <class I>
inline void Omega::MetaInfo::iface_stub_functor<I*>::init(typename interface_info<I>::safe_class* pS, const guid_t& iid)
{
	if (pS)
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = pS->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

		try 
		{
			m_pI = static_cast<I*>(lookup_proxy(pObjS,iid,false));
			pObjS->Release_Safe();
		}
		catch (...)
		{
			if (pObjS)
				pObjS->Release_Safe();
			throw;
		}
	}
}

template <class I>
inline void Omega::MetaInfo::iface_stub_functor<I*>::detach(typename interface_info<I>::safe_class*& result, const guid_t& iid)
{
	if (result)
	{
		IException_Safe* pSE = result->Release_Safe();
		if (pSE)
			throw_correct_exception(pSE);
	}
	
	if (!m_pI)
		result = 0;
	else
	{
		IObject* pObj = m_pI->QueryInterface(IID_IObject);
		if (!pObj)
			INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

		try
		{
			result = static_cast<typename interface_info<I>::safe_class*>(lookup_stub(pObj,iid));
			pObj->Release();
		}
		catch (...)
		{
			if (pObj)
				pObj->Release();
			throw;
		}
	}
}

template <class I>
inline void Omega::MetaInfo::iface_proxy_functor<I*>::init(I* pI, const guid_t& iid)
{
	if (pI)
	{
		IObject* pObj = 0;
		try
		{
			pObj = pI->QueryInterface(IID_IObject);
			if (!pObj)
				INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

			m_pS = static_cast<typename interface_info<I*>::safe_class>(lookup_stub(pObj,iid));
			pObj->Release();
		}
		catch (...)
		{
			if (pObj)
				pObj->Release();
			throw;
		}
	}
}

template <class I>
inline void Omega::MetaInfo::iface_proxy_functor<I*>::detach(I*& result, const guid_t& iid)
{
	if (result)
		result->Release();

	if (!m_pS)
		result = 0;
	else
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = m_pS->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

		try 
		{
			result = static_cast<I*>(lookup_proxy(pObjS,iid,true));
			pObjS->Release_Safe();
		}
		catch (...)
		{
			if (pObjS)
				pObjS->Release_Safe();
			throw;
		}
	}
}

template <class I>
inline Omega::MetaInfo::iface_stub_functor_array<I>::~iface_stub_functor_array()
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
void inline Omega::MetaInfo::iface_stub_functor_array<I>::init(typename interface_info<I>::safe_class* pVals)
{
	try
	{
		if (m_cbSize>0)
		{
			OMEGA_NEW(m_pFunctors,interface_info<I>::stub_functor[m_cbSize]);
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
inline Omega::MetaInfo::iface_proxy_functor_array<I>::~iface_proxy_functor_array()
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
inline void Omega::MetaInfo::iface_proxy_functor_array<I>::init(I* pVals)
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

inline Omega::MetaInfo::IException_Safe* OMEGA_CALL Omega::MetaInfo::SafeStub::QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid)
{
	if (iid==IID_IObject)
	{
		*retval = this;
		return (*retval)->AddRef_Safe();
	}

	*retval = 0;
	try
	{
		IObject_Safe* pObjS = 0;
		try
		{
			Guard<CriticalSection> guard(m_cs);
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
						pSE = i->second->AddRef_Safe();
						if (pSE)
						{
							pObjS->Release_Safe();
							return pSE;
						}

						i = m_iid_map.insert(std::map<const guid_t,IObject_Safe*>::value_type(iid,i->second)).first;
						break;
					}
				}
				
				if (i == m_iid_map.end())
				{
					// New interface required
					const qi_rtti* pRtti = get_qi_rtti_info(iid);
					if (!pRtti || !pRtti->pfnCreateSafeStub)
						OMEGA_THROW("No handler for interface");

					pObjS = pRtti->pfnCreateSafeStub(this,m_pObj);
					i = m_iid_map.insert(std::map<const guid_t,IObject_Safe*>::value_type(iid,pObjS)).first;
				}
			}
			
			if (i->second)
				return i->second->QueryInterface_Safe(retval,iid);
		}
		catch (std::exception& e)
		{
			if (pObjS)
				pObjS->Release_Safe();
			OMEGA_THROW(e.what());
		}
		catch (IException*)
		{
			if (pObjS)
				pObjS->Release_Safe();
			throw;
		}
		catch (...)
		{
			if (pObjS)
				pObjS->Release_Safe();
			OMEGA_THROW("Unexpected exception type caught!");
		}
	}
	catch (IException* pE)
	{
		return return_safe_exception(pE);
	}
	
	return 0;
}

inline Omega::IObject* Omega::MetaInfo::SafeProxy::QueryInterface(const guid_t& iid)
{
	if (iid==IID_IObject ||
		iid==IID_SafeProxy)
	{
		AddRef();
		return this;
	}

	IObject* pObj = 0;
	try
	{
		Guard<CriticalSection> guard(m_cs);
		std::map<const guid_t,IObject*>::iterator i=m_iid_map.find(iid);
		if (i == m_iid_map.end())
		{
			// QI all entries
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				pObj = i->second->QueryInterface(iid);
				if (pObj)
				{
					// If we find one, add it to the map...
					i = m_iid_map.insert(std::map<const guid_t,IObject*>::value_type(iid,i->second)).first;
					break;
				}
			}
			
			if (i == m_iid_map.end())
			{
				// New interface required
				const qi_rtti* pRtti = get_qi_rtti_info(iid);
				if (!pRtti && !pRtti->pfnCreateSafeProxy)
					OMEGA_THROW("No handler for interface");
				
				pObj = pRtti->pfnCreateSafeProxy(this,m_pS);
				i = m_iid_map.insert(std::map<const guid_t,IObject*>::value_type(iid,pObj)).first;
			}
		}

		if (i->second)
			return i->second->QueryInterface(iid);
	}
	catch (std::exception& e)
	{
		if (pObj)
			pObj->Release();
		OMEGA_THROW(e.what());
	}
	
	return 0;
}

inline const Omega::MetaInfo::qi_rtti* Omega::MetaInfo::get_qi_rtti_info(const guid_t& iid)
{
	static std::map<const guid_t,const qi_rtti*> mapRtti;
	static CriticalSection cs;
	
	Guard<CriticalSection> guard(cs);
	
	std::map<const guid_t,const qi_rtti*>::iterator i=mapRtti.find(iid);
	if (i==mapRtti.end())
	{
		const qi_rtti* pRet = 0;
		get_qi_rtti_info_impl<(sizeof(get_qi_rtti(&pRet,(size_t_<0>::type*)0,iid)) == sizeof(yes_t))>::execute(&pRet,(size_t_<0>*)0,iid);
		i = mapRtti.insert(std::map<const guid_t,const qi_rtti*>::value_type(iid,pRet)).first;
	}

	return i->second;
};

inline Omega::MetaInfo::SafeProxyStubMap& Omega::MetaInfo::get_proxy_map()
{
	static SafeProxyStubMap proxy_map;
	return proxy_map;
}

inline Omega::MetaInfo::SafeProxyStubMap& Omega::MetaInfo::get_stub_map()
{
	static SafeProxyStubMap stub_map;
	return stub_map;
}

inline Omega::MetaInfo::IObject_Safe* Omega::MetaInfo::lookup_stub(Omega::IObject* pObj, const Omega::guid_t& iid)
{
	SafeProxyStubMap& stub_map = get_stub_map();
	Guard<CriticalSection> guard(stub_map.m_cs);

	bool bRelease = false;
	IObject_Safe* pSafeStub = 0;
	try
	{
		std::map<void*,void*>::iterator i=stub_map.m_map.find(pObj);
		if (i != stub_map.m_map.end())
			pSafeStub = static_cast<IObject_Safe*>(i->second);
		else
		{
			SafeProxy* pSafeProxy = static_cast<SafeProxy*>(pObj->QueryInterface(IID_SafeProxy));
			if (pSafeProxy)
			{
				pSafeStub = pSafeProxy->GetSafeStub();
				pSafeProxy->Release();
			}
			else
			{
                OMEGA_NEW(pSafeStub,SafeStub(pObj));
				bRelease = true;
			}
			
			stub_map.m_map.insert(std::map<void*,void*>::value_type(pObj,pSafeStub));
		}
	}
	catch (std::exception& e)
	{
		if (pSafeStub && bRelease)
			pSafeStub->Release_Safe();
		OMEGA_THROW(e.what());
	}

	IObject_Safe* pRet = 0;
	IException_Safe* pSE = pSafeStub->QueryInterface_Safe(&pRet,iid);
	if (bRelease)
		pSafeStub->Release_Safe();

	if (pSE)
		throw_correct_exception(pSE);

	if (!pRet)
		Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
	
	return pRet;
}

inline Omega::IObject* Omega::MetaInfo::lookup_proxy(Omega::MetaInfo::IObject_Safe* pObjS, const Omega::guid_t& iid, bool bPartialAllowed)
{
	SafeProxyStubMap& proxy_map = get_proxy_map();
	Guard<CriticalSection> guard(proxy_map.m_cs);

	bool bRelease = false;
	IObject* pSafeProxy = 0;
	try
	{
		std::map<void*,void*>::iterator i=proxy_map.m_map.find(pObjS);
		if (i != proxy_map.m_map.end())
			pSafeProxy = static_cast<IObject*>(i->second);
		else
		{
			OMEGA_NEW(pSafeProxy,SafeProxy(pObjS));
			proxy_map.m_map.insert(std::map<void*,void*>::value_type(pObjS,pSafeProxy));
			bRelease = true;
		}
	}
	catch (std::exception& e)
	{
		if (pSafeProxy && bRelease)
			pSafeProxy->Release();
		OMEGA_THROW(e.what());
	}

	IObject* pRet = pSafeProxy->QueryInterface(iid);
	if (!pRet)
	{
		if (bPartialAllowed)
		{
			pRet = pSafeProxy;
			pRet->AddRef();
		}
		else
			INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
	}

	if (bRelease)
		pSafeProxy->Release();

	return pRet;
}

inline Omega::MetaInfo::IException_Safe* Omega::MetaInfo::return_safe_exception(Omega::IException* pE)
{
	// Wrap with the correct _SafeStub wrapper by calling QI
	IObject_Safe* pSE2 = 0;
	IException_Safe* pSE3 = static_cast<IException_Safe*>(interface_info<IException*>::proxy_functor(pE))->QueryInterface_Safe(&pSE2,pE->ActualIID());
	pE->Release();

	if (pSE3)
		return pSE3;
	if (!pSE2)
	{
		try
		{
			INoInterfaceException::Throw(pE->ActualIID(),OMEGA_FUNCNAME);
		}
		catch (IException* pE2)
		{
			return return_safe_exception(pE2);
		}
	}
	
	return static_cast<IException_Safe*>(pSE2);
}

inline void Omega::MetaInfo::throw_correct_exception(IException_Safe* pSE)
{
	guid_t iid;
	IException_Safe* pSE2 = pSE->ActualIID_Safe(&iid);
	if (pSE2)
		throw_correct_exception(pSE2);
	else
	{
		const qi_rtti* pRtti = get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnSafeThrow)
			OMEGA_THROW("No throw handler for exception interface");
		
		pRtti->pfnSafeThrow(pSE);
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(IException_Throw,3,((in),const Omega::char_t*,desc,(in),const Omega::char_t*,source,(in),Omega::IException*,pCause));
inline void Omega::IException::Throw(const Omega::char_t* desc, const Omega::char_t* source, Omega::IException* pCause)
{
	IException_Throw(desc,source,pCause);
}

#endif // OOCORE_RTTI_INL_INCLUDED_

