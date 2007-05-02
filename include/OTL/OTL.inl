#ifndef OTL_BASE_INL_INCLUDED_
#define OTL_BASE_INL_INCLUDED_

size_t OTL::ModuleBase::GetLockCount() const
{
	return static_cast<size_t>(m_lockCount.value());
}

void OTL::ModuleBase::IncLockCount()
{
	++m_lockCount;
}

void OTL::ModuleBase::DecLockCount()
{
	--m_lockCount;
}

Omega::Guard<Omega::CriticalSection> OTL::ModuleBase::GetGuard()
{
	return Omega::Guard<Omega::CriticalSection>(m_csMain);
}

void OTL::ModuleBase::AddTermFunc(OTL::ModuleBase::TERM_FUNC pfnTerm, void* arg)
{
	try
	{
		Omega::Guard<Omega::CriticalSection> lock(m_csMain);

		Term term = { pfnTerm, arg };

		m_listTerminators.push_front(term);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

OTL::ModuleBase::~ModuleBase()
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

Omega::Activation::IObjectFactory* OTL::LibraryModule::GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags)
{
	Omega::IObject* pObject = 0;
    const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		if (*(g[i].pfnOid)() == oid)
		{
			pObject = g[i].pfnCreate(Omega::Activation::IID_IObjectFactory,flags);
			break;
		}
	}
	return static_cast<Omega::Activation::IObjectFactory*>(pObject);
}

#endif  // OTL_BASE_INL_INCLUDED_
