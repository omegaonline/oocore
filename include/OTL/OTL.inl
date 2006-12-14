#ifndef OTL_BASE_INL_INCLUDED_
#define OTL_BASE_INL_INCLUDED_

inline size_t OTL::ModuleBase::GetLockCount() const
{
	return static_cast<size_t>(m_lockCount.value());
}

inline void OTL::ModuleBase::IncLockCount()
{
	++m_lockCount;
}

inline void OTL::ModuleBase::DecLockCount()
{
	--m_lockCount;
}

inline Omega::Guard<Omega::CriticalSection> OTL::ModuleBase::GetGuard()
{
	return Omega::Guard<Omega::CriticalSection>(m_csMain);
}

inline void OTL::ModuleBase::AddTermFunc(OTL::ModuleBase::TERM_FUNC pfnTerm, void* arg)
{
	Term term = { pfnTerm, arg };

	Omega::Guard<Omega::CriticalSection> lock(m_csMain);
	try
	{
		m_listTerminators.push_front(term);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

inline OTL::ModuleBase::~ModuleBase()
{
	try
	{
		for (std::list<Term>::iterator i=m_listTerminators.begin(); i!=m_listTerminators.end(); ++i)
		{
			try
			{
				i->pfn(i->arg);
			}
			catch (...)
			{}
		}
		m_listTerminators.clear();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

inline void OTL::LibraryModule::GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid, Omega::IObject** ppObject)
{
	*ppObject = 0;
    const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		if (*(g[i].pfnOid)() == oid)
		{
			*ppObject = g[i].pfnCreate(iid,flags);
			break;
		}
	}
}

#endif  // OTL_BASE_INL_INCLUDED_