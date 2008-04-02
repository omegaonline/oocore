///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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
		OMEGA_THROW(e);
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
		OMEGA_THROW(e);
	}
}

Omega::IObject* OTL::LibraryModule::GetLibraryObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid)
{
	// We ignore any registered flags, and only enforce InProcess creation, because we are a library!
	if (!(flags & Omega::Activation::InProcess))
		return 0;

    const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
	    if (*(g[i].pfnOid)() == oid)
		{
			return g[i].pfnCreate(iid,flags);
		}
	}
	return 0;
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

			if (g[i].bLocal)
				strXML += L"<key name=\"\\Local User\\Objects\">";
			else
				strXML += L"<key name=\"\\Objects\">";

			strXML +=
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

void OTL::ProcessModule::RegisterObjectsImpl(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strAppName, const Omega::string_t& strSubsts)
{
	Omega::string_t strXML;
	
	if (bLocal)
		strXML = L"<key name=\"\\Local User\\Applications\">";
	else
		strXML = L"<key name=\"\\Applications\">";

	strXML += 	L"<key name=\"" + strAppName + L"\" uninstall=\"Remove\">"
					L"<key name=\"Activation\">"
						L"<value name=\"Path\">%MODULE_PATH%</value>"
					L"</key>"
				L"</key>"
			L"</key>";
	
	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		if (g[i].pszName != 0)
		{
			Omega::string_t strName = g[i].pszName;
			Omega::string_t strOID = (g[i].pfnOid)()->ToString();

			if (g[i].bLocal)
				strXML += L"<key name=\"\\Local User\\Objects\">";
			else
				strXML += L"<key name=\"\\Objects\">";

			strXML +=
					L"<key name=\"" + strName + L"\" uninstall=\"Remove\">"
						L"<value name=\"OID\">" + strOID + L"</value>"
					L"</key>"
					L"<key name=\"OIDs\">"
						L"<key name=\"" + strOID + L"\" uninstall=\"Remove\">"
							L"<value name=\"Application\">" + strAppName + L"</value>";

			if (false)
				strXML +=	L"<value name=\"Public\" type=\"Integer\">1</value>";

			strXML +=	L"</key>"
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
	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		ObjectPtr<Omega::Activation::IObjectFactory> ptrOF;
		ptrOF.Attach(static_cast<Omega::Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_UUIDOF(Omega::Activation::IObjectFactory),Omega::Activation::InProcess)));

		g[i].cookie = Omega::Activation::RegisterObject(*(g[i].pfnOid)(),ptrOF,(g[i].pfnActivationFlags)(),(g[i].pfnRegistrationFlags)());
	}
}

void OTL::ProcessModule::UnregisterObjectFactories()
{
	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		Omega::Activation::RevokeObject(g[i].cookie);
		g[i].cookie = 0;
	}
}

void OTL::ProcessModule::Run()
{
	RegisterObjectFactories();

	try
	{
		do
		{
			Omega::HandleRequests(30000);
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
