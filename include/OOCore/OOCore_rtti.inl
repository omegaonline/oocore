#ifndef OOCORE_RTTI_INL_INCLUDED_
#define OOCORE_RTTI_INL_INCLUDED_

template <class I>
inline Omega::MetaInfo::stub_functor<I>::stub_functor(I* pI)
{
	if (!pI)
		m_pS = 0;
	else
	{
		IObject* pObj = 0;
		try
		{
			pObj = pI->QueryInterface(IID_IObject);
			if (!pObj)
				INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

			m_pS = static_cast<typename interface_info<I*>::safe>(lookup_stub(pObj,iid_traits<I>::GetIID()));
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
inline Omega::MetaInfo::stub_functor_out<I>::stub_functor_out(I** ppI, const guid_t& iid = iid_traits<I>::GetIID()) : 
	m_ppI(ppI), m_iid(iid)
{
	if (!*ppI)
		m_pS = 0;
	else
	{
		IObject* pObj = 0;
		try
		{
			pObj = (*ppI)->QueryInterface(IID_IObject);
			if (!pObj)
				INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

			m_pS = static_cast<typename interface_info<I*>::safe>(lookup_stub(pObj,m_iid));
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
inline Omega::MetaInfo::stub_functor_out<I>::~stub_functor_out()
{
	if (m_ppI)
	{
		if (*m_ppI)
			(*m_ppI)->Release();

		if (!m_pS)
			*m_ppI = 0;
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
				(*m_ppI) = static_cast<I*>(lookup_proxy(pObjS,m_iid,true));
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
	if (m_pS)
		m_pS->Release_Safe();
}

template <class I>
inline Omega::MetaInfo::proxy_functor<I>::proxy_functor(typename interface_info<I*>::safe pS)
{
	if (!pS)
		m_pI = 0;
	else
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = pS->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

		try 
		{
			m_pI = static_cast<I*>(lookup_proxy(pObjS,iid_traits<I>::GetIID(),false));
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
inline Omega::MetaInfo::proxy_functor_out<I>::proxy_functor_out(typename interface_info<I**>::safe ppS, const guid_t& iid = iid_traits<I>::GetIID()) : 
	m_ppS(ppS), m_iid(iid)
{
	if (!*ppS)
		m_pI = 0;
	else
	{
		IObject_Safe* pObjS = 0;
		IException_Safe* pSE = (*ppS)->QueryInterface_Safe(&pObjS,IID_IObject);
		if (pSE)
			throw_correct_exception(pSE);
		if (!pObjS)
			INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

		try 
		{
			m_pI = static_cast<I*>(lookup_proxy(pObjS,m_iid,false));
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
inline Omega::MetaInfo::proxy_functor_out<I>::~proxy_functor_out()
{
	if (m_ppS)
	{
		if (*m_ppS)
		{
			IException_Safe* pSE = (*m_ppS)->Release_Safe();
			if (pSE)
				throw_correct_exception(pSE);
		}

		if (!m_pI)
			*m_ppS = 0;
		else
		{
			IObject* pObj = m_pI->QueryInterface(IID_IObject);
			if (!pObj)
				INoInterfaceException::Throw(IID_IObject,OMEGA_FUNCNAME);

			try
			{
				*m_ppS = static_cast<typename interface_info<I*>::safe>(lookup_stub(pObj,m_iid));
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
	if (m_pI)
		m_pI->Release();
}

inline Omega::MetaInfo::IException_Safe* OMEGA_CALL Omega::MetaInfo::Stub::QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid)
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
					if (pRtti && pRtti->pfnCreateStub)
						pObjS = pRtti->pfnCreateStub(this,m_pObj);
													
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
		return return_correct_exception(pE);
	}
	
	return 0;
}

inline Omega::IObject* Omega::MetaInfo::Proxy::QueryInterface(const guid_t& iid)
{
	if (iid==IID_IObject ||
		iid==IID_Proxy)
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
				if (pRtti && pRtti->pfnCreateProxy)
					pObj = pRtti->pfnCreateProxy(this,m_pS);
									
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

inline Omega::MetaInfo::ProxyStubMap& Omega::MetaInfo::get_proxy_map()
{
	static ProxyStubMap proxy_map;
	return proxy_map;
}

inline Omega::MetaInfo::ProxyStubMap& Omega::MetaInfo::get_stub_map()
{
	static ProxyStubMap stub_map;
	return stub_map;
}

inline Omega::MetaInfo::IObject_Safe* Omega::MetaInfo::lookup_stub(Omega::IObject* pObj, const Omega::guid_t& iid)
{
	ProxyStubMap& stub_map = get_stub_map();
	Guard<CriticalSection> guard(stub_map.m_cs);

	bool bRelease = false;
	IObject_Safe* pStub = 0;
	try
	{
		std::map<void*,void*>::iterator i=stub_map.m_map.find(pObj);
		if (i != stub_map.m_map.end())
			pStub = static_cast<IObject_Safe*>(i->second);
		else
		{
			Proxy* pProxy = static_cast<Proxy*>(pObj->QueryInterface(IID_Proxy));
			if (pProxy)
			{
				pStub = pProxy->GetStub();
				pProxy->Release();
			}
			else
			{
                OMEGA_NEW(pStub,Stub(pObj));
				bRelease = true;
			}
			
			stub_map.m_map.insert(std::map<void*,void*>::value_type(pObj,pStub));
		}
	}
	catch (std::exception& e)
	{
		if (pStub && bRelease)
			pStub->Release_Safe();
		OMEGA_THROW(e.what());
	}

	IObject_Safe* pRet = 0;
	IException_Safe* pSE = pStub->QueryInterface_Safe(&pRet,iid);
	if (bRelease)
		pStub->Release_Safe();

	if (pSE)
		throw_correct_exception(pSE);

	if (!pRet)
		Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
	
	return pRet;
}

inline Omega::IObject* Omega::MetaInfo::lookup_proxy(Omega::MetaInfo::IObject_Safe* pObjS, const Omega::guid_t& iid, bool bPartialAllowed)
{
	ProxyStubMap& proxy_map = get_proxy_map();
	Guard<CriticalSection> guard(proxy_map.m_cs);

	bool bRelease = false;
	IObject* pProxy = 0;
	try
	{
		std::map<void*,void*>::iterator i=proxy_map.m_map.find(pObjS);
		if (i != proxy_map.m_map.end())
			pProxy = static_cast<IObject*>(i->second);
		else
		{
			OMEGA_NEW(pProxy,Proxy(pObjS));
			proxy_map.m_map.insert(std::map<void*,void*>::value_type(pObjS,pProxy));
			bRelease = true;
		}
	}
	catch (std::exception& e)
	{
		if (pProxy && bRelease)
			pProxy->Release();
		OMEGA_THROW(e.what());
	}

	IObject* pRet = pProxy->QueryInterface(iid);
	if (!pRet)
	{
		if (bPartialAllowed)
		{
			pRet = pProxy;
			pRet->AddRef();
		}
		else
			INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
	}

	if (bRelease)
		pProxy->Release();

	return pRet;
}

inline Omega::MetaInfo::IException_Safe* Omega::MetaInfo::return_correct_exception(Omega::IException* pE)
{
	// Wrap with the correct _Stub wrapper by calling QI
	IObject_Safe* pSE2 = 0;
	IException_Safe* pSE3 = static_cast<IException_Safe*>(interface_info<IException*>::stub(pE))->QueryInterface_Safe(&pSE2,pE->GetIID());
	pE->Release();

	if (pSE3)
		return pSE3;
	if (!pSE2)
	{
		try
		{
			INoInterfaceException::Throw(pE->GetIID(),OMEGA_FUNCNAME);
		}
		catch (IException* pE2)
		{
			return return_correct_exception(pE2);
		}
	}
	
	return static_cast<IException_Safe*>(pSE2);
}

inline void Omega::MetaInfo::throw_correct_exception(IException_Safe* pSE)
{
	guid_t iid;
	IException_Safe* pSE2 = pSE->GetIID_Safe(&iid);
	if (pSE2)
		throw_correct_exception(pSE2);
	else
	{
		const qi_rtti* pRtti = get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnThrow)
			OMEGA_THROW("No throw handler for interface");
		else
			pRtti->pfnThrow(pSE);
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(IException_Throw,3,((in),const Omega::char_t*,desc,(in),const Omega::char_t*,source,(in),Omega::IException*,pCause));
inline void Omega::IException::Throw(const Omega::char_t* desc, const Omega::char_t* source, Omega::IException* pCause)
{
	IException_Throw(desc,source,pCause);
}

#endif // OOCORE_RTTI_INL_INCLUDED_
