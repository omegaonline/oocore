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

Omega::System::CriticalSection& OTL::ModuleBase::GetLock()
{
	return m_csMain;
}

void OTL::ModuleBase::AddTermFunc(OTL::ModuleBase::TERM_FUNC pfnTerm, void* arg)
{
	try
	{
		Omega::System::Guard lock(m_csMain);

		Term term = { pfnTerm, arg };

		m_listTerminators.push_front(term);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(Omega::string_t(e.what(),false));
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
		OMEGA_THROW(Omega::string_t(e.what(),false));
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
			pObject = g[i].pfnCreate(OMEGA_UUIDOF(Omega::Activation::IObjectFactory),flags);
			break;
		}
	}
	return static_cast<Omega::Activation::IObjectFactory*>(pObject);
}

void OTL::LibraryModule::RegisterLibrary(Omega::bool_t bInstall, const Omega::string_t& strSubsts)
{
	Omega::string_t strXML;		

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		if (g[i].pszName != 0)
		{
			Omega::string_t strName = g[i].pszName;
			Omega::string_t strOID = (g[i].pfnOid)()->ToString();

			strXML += 
				L"<key name=\"Objects\">"
					L"<key name=\"" + strName + L"\" uninstall=\"Remove\">"
						L"<value name=\"OID\">" + strOID + L"</value>"
					L"</key>"
					L"<key name=\"OIDs\">"
						L"<key name=\"" + strOID + L"\" uninstall=\"Remove\">"
							L"<value name=\"Library\">%LIB_PATH%</value>"
						L"</key>"
					L"</key>"
				L"</key>";
		}		
	}

	if (!strXML.IsEmpty())
	{
		strXML = L"<?xml version=\"1.0\"?>"
				 L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				 + strXML +
				 L"</root>";

		Omega::Registry::AddXML(strXML,bInstall,strSubsts);
	}
}

void OTL::ProcessModule::RegisterObjectsImpl(Omega::bool_t bInstall, const Omega::string_t& strAppName, const Omega::string_t& strSubsts)
{
	Omega::string_t strXML =
		L"<key name=\"Applications\">"
			L"<key name=\"" + strAppName + L"\" uninstall=\"Remove\">"
				L"<value name=\"Activation\">%MODULE_PATH%</value>"
			L"</key>"
		L"</key>";

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		if (g[i].pszName != 0)
		{
			Omega::string_t strName = g[i].pszName;
			Omega::string_t strOID = (g[i].pfnOid)()->ToString();

			strXML += 
				L"<key name=\"Objects\">"
					L"<key name=\"" + strName + L"\" uninstall=\"Remove\">"
						L"<value name=\"OID\">" + strOID + L"</value>"
					L"</key>"
					L"<key name=\"OIDs\">"
						L"<key name=\"" + strOID + L"\" uninstall=\"Remove\">"
							L"<value name=\"Application\">" + strAppName + L"</value>"
						L"</key>"
					L"</key>"
				L"</key>";
		}		
	}

	if (!strXML.IsEmpty())
	{
		strXML = L"<?xml version=\"1.0\"?>"
				 L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
				 + strXML +
				 L"</root>";

		Omega::Registry::AddXML(strXML,bInstall,strSubsts);
	}
}

void OTL::ProcessModule::RegisterObjectFactories()
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT;
	ptrROT.Attach(Omega::Activation::IRunningObjectTable::GetRunningObjectTable());

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		ObjectPtr<Omega::Activation::IObjectFactory> ptrOF;

		void* TODO;	// flags need sorting

		ptrOF.Attach(static_cast<Omega::Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_UUIDOF(Omega::Activation::IObjectFactory),0)));
		ptrROT->Register(*(g[i].pfnOid)(),Omega::Activation::IRunningObjectTable::Default,ptrOF);
	}
}

void OTL::ProcessModule::UnregisterObjectFactories()
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT;
	ptrROT.Attach(Omega::Activation::IRunningObjectTable::GetRunningObjectTable());

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		ptrROT->Revoke(*(g[i].pfnOid)());
	}
}

void OTL::ProcessModule::Run()
{
	RegisterObjectFactories();
	
	try
	{
		do
		{
#ifdef OMEGA_DEBUG
			Omega::HandleRequests();
#else
			Omega::HandleRequests(60000);
#endif
		} while (GetLockCount() > 0);
	}
	catch (Omega::IException* pE)
	{
		UnregisterObjectFactories();
		throw pE;
	}
	
	UnregisterObjectFactories();
}

#endif  // OTL_BASE_INL_INCLUDED_
