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

#ifndef OTL_BASE_INL_INCLUDED_
#define OTL_BASE_INL_INCLUDED_

bool OTL::ModuleBase::HaveLocks() const
{
	return !m_lockCount.IsZero();
}

void OTL::ModuleBase::IncLockCount()
{
	m_lockCount.AddRef();
}

void OTL::ModuleBase::DecLockCount()
{
	m_lockCount.Release();
}

Omega::Threading::Mutex& OTL::ModuleBase::GetLock()
{
	return m_csMain;
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

void OTL::LibraryModule::RegisterLibrary(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts)
{
	Omega::string_t strXML;

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		Omega::string_t strOID = (g[i].pfnOid)()->ToString();

		if (bLocal)
			strXML += L"<key name=\"\\Local User\\Objects\">";
		else
			strXML += L"<key name=\"\\All Users\\Objects\">";

		if (g[i].pszName != 0)
		{
			Omega::string_t strName = g[i].pszName;

			strXML +=
					L"<key name=\"" + strName + L"\" uninstall=\"Remove\">"
						L"<value name=\"OID\">" + strOID + L"</value>"
					L"</key>";
		}

		strXML +=
					L"<key name=\"OIDs\">"
						L"<key name=\"" + strOID + L"\" uninstall=\"Remove\">"
							L"<value name=\"Library\">%LIB_PATH%</value>"
						L"</key>"
					L"</key>"
				L"</key>";
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

void OTL::ProcessModule::InstallObjectsImpl(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strAppName, const Omega::string_t& strSubsts)
{
	if (strAppName.IsEmpty())
		return;

	Omega::string_t strXML;

	if (bLocal)
		strXML = L"<key name=\"\\Local User\\Applications\">";
	else
		strXML = L"<key name=\"\\All Users\\Applications\">";

	strXML += 	L"<key name=\"" + strAppName + L"\" uninstall=\"Remove\">"
					L"<key name=\"Activation\">"
						L"<value name=\"Path\">%MODULE_PATH%</value>"
					L"</key>"
				L"</key>"
			L"</key>";

	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0;g[i].pfnOid!=0;++i)
	{
		Omega::string_t strOID = (g[i].pfnOid)()->ToString();

		if (bLocal)
			strXML += L"<key name=\"\\Local User\\Objects\">";
		else
			strXML += L"<key name=\"\\All Users\\Objects\">";

		if (g[i].pszName != 0)
		{
			Omega::string_t strName = g[i].pszName;

			strXML +=
					L"<key name=\"" + strName + L"\" uninstall=\"Remove\">"
						L"<value name=\"OID\">" + strOID + L"</value>"
					L"</key>";
		}

		strXML +=
				L"<key name=\"OIDs\">"
					L"<key name=\"" + strOID + L"\" uninstall=\"Remove\">"
						L"<value name=\"Application\">" + strAppName + L"</value>"
					L"</key>"
				L"</key>"
			L"</key>";
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
		ptrOF.Attach(static_cast<Omega::Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_GUIDOF(Omega::Activation::IObjectFactory),Omega::Activation::InProcess)));

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
		while (!Omega::HandleRequest(30000) || HaveLocks())
		{}
	}
	catch (...)
	{
		UnregisterObjectFactories();
		throw;
	}

	UnregisterObjectFactories();
}

#endif  // OTL_BASE_INL_INCLUDED_
