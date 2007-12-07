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

#include "OOCore_precomp.h"

#if defined(OMEGA_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include "./Xml.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	ObjectPtr<Remoting::IInterProcessService> GetInterProcessService();
	ObjectPtr<Registry::IRegistryKey> GetRootKey();

	void ReadXmlKey(const wchar_t*& rd_ptr, ObjectPtr<Registry::IRegistryKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	void ReadXmlKeyContents(const wchar_t*& rd_ptr, ObjectPtr<Registry::IRegistryKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	ObjectPtr<Registry::IRegistryKey> ProcessXmlKeyAttribs(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IRegistryKey> ptrKey, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	void ProcessXmlValue(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IRegistryKey> ptrKey, const string_t& strData, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	string_t SubstituteNames(const string_t& strName, const std::map<string_t,string_t>& mapSubsts);

	const wchar_t xmlns[] = L"http://www.omegaonline.org.uk/schemas/registry.xsd";
}

ObjectPtr<Registry::IRegistryKey> OOCore::GetRootKey()
{
	ObjectPtr<Registry::IRegistryKey> ptrKey;
	ObjectPtr<Remoting::IInterProcessService> ptrIPS = OOCore::GetInterProcessService();
	ptrKey.Attach(static_cast<Registry::IRegistryKey*>(ptrIPS->GetRegistry()));
	return ptrKey;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IRegistryKey::OpenFlags_t,flags))
{
	ObjectPtr<Registry::IRegistryKey> ptrKey;
	ptrKey.Attach(static_cast<Registry::IRegistryKey*>(OOCore::GetInterProcessService()->GetRegistry()));
	return ptrKey->OpenSubKey(key,flags);
}

string_t OOCore::SubstituteNames(const string_t& strName, const std::map<string_t,string_t>& mapSubsts)
{
	string_t strRes1 = strName;
	
	for (std::map<string_t,string_t>::const_iterator i=mapSubsts.begin();i!=mapSubsts.end();++i)
	{
		string_t strRes2;
		const wchar_t* p1 = strRes1.c_str();
		for (;;)
		{
			const wchar_t* p2 = ACE_OS::strstr(p1,i->first.c_str());
			if (!p2)
			{
				strRes2 += string_t(p1);
				break;
			}
			
			strRes2 += string_t(p1,p2 - p1) + i->second;
			p1 = p2 + i->first.Length();
		}

		strRes1 = strRes2;
	}

	return strRes1;
}

ObjectPtr<Registry::IRegistryKey> OOCore::ProcessXmlKeyAttribs(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IRegistryKey> ptrKey, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
{
	std::map<string_t,string_t>::const_iterator i = attribs.find(L"name");
	if (i == attribs.end())
		OMEGA_THROW(L"Expected name attribute not found");

	string_t strName = SubstituteNames(i->second,mapSubsts);

	if (bAdd)
		return ptrKey.OpenSubKey(strName,Registry::IRegistryKey::Create);
	else
	{
		bool bRemove = false;
		i = attribs.find(L"uninstall");
		if (i != attribs.end() && i->second==L"Remove")
			bRemove = true;

		if (ptrKey && ptrKey->IsSubKey(strName))
		{
			if (bRemove)
				ptrKey->DeleteKey(strName);
			else
				return ptrKey.OpenSubKey(strName,Registry::IRegistryKey::OpenExisting);
		}
	}

	return 0;
}

void OOCore::ProcessXmlValue(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IRegistryKey> ptrKey, const string_t& strData, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
{
	std::map<string_t,string_t>::const_iterator i = attribs.find(L"name");
	if (i == attribs.end())
		OMEGA_THROW(L"Expected name attribute not found");

	string_t strName = SubstituteNames(i->second,mapSubsts);
		
	string_t strType = L"String";
	i = attribs.find(L"type");
	if (i != attribs.end())
		strType = i->second;

	if (!bAdd)
	{
		bool bRemove = true;
		i = attribs.find(L"uninstall");
		if (i != attribs.end() && i->second!=L"Remove")
			bRemove = false;

        if (bRemove && ptrKey && ptrKey->IsValue(strName))
			ptrKey->DeleteValue(strName);
	}
	else
	{
		string_t strData2 = SubstituteNames(strData,mapSubsts);

		if (strType == L"String")
			ptrKey->SetStringValue(strName,strData2);
		else if (strType == L"UInt32")
		{
			// Skip the starting whitespace
			const wchar_t* p = strData2.c_str();
			p += ACE_OS::strspn(p,L" ");
			if (p[0]==L'0' && p[1]==L'x')
				ptrKey->SetUIntValue(strName,ACE_OS::strtoul(p+2,0,16));
			else
				ptrKey->SetUIntValue(strName,ACE_OS::strtoul(p,0,10));
		}
		else if (strType == L"Binary")
		{
			OMEGA_THROW(L"Binary values are not supported");
		}
		else
		{
			OMEGA_THROW(L"Invalid value type");
		}
	}
}

void OOCore::ReadXmlKeyContents(const wchar_t*& rd_ptr, ObjectPtr<Registry::IRegistryKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
{
	std::map<string_t,string_t> attribs;
	bool bHasContent;
	string_t strName;
	Xml::ParseXMLElement(rd_ptr,strName,bHasContent,attribs);
	
	// Get out the namespaces
	std::map<string_t,string_t> namespaces2 = namespaces;
	Xml::ParseXMLNamespaces(attribs,namespaces2);

	// Check namespace...
	string_t strName2 = strName;
	string_t strNSpace;
	Xml::ParseXMLName(strName2,strNSpace,namespaces2);
	if (namespaces2[strNSpace] != xmlns)
    	OMEGA_THROW(L"Invalid schema");

	// Check what we have found...
	if (strName2 != L"key" && strName2 != L"value")
		OMEGA_THROW(L"key or value element expected");

	if (strName2 == L"key")
	{
		// Sort out attributes...
		ObjectPtr<Registry::IRegistryKey> ptrSubKey = ProcessXmlKeyAttribs(attribs,ptrKey,bAdd,mapSubsts);

		if (bHasContent)
		{
			// Skip any leading guff..
			string_t strGuff;
			Xml::ParseXMLCharData(rd_ptr,strGuff);	
			
			// Read key contents...
			while (ACE_OS::strncmp(rd_ptr,L"</",2) != 0)
			{
				ReadXmlKeyContents(rd_ptr,ptrSubKey,namespaces2,bAdd,mapSubsts);

				// Skip any guff...
				Xml::ParseXMLCharData(rd_ptr,strGuff);
			} 

			Xml::ParseXMLEndElement(rd_ptr,strName);
		}
	}
	else
	{
		// Read content
		string_t strData;

		if (bHasContent)
		{
			Xml::ParseXMLCharData(rd_ptr,strData);	
			Xml::ParseXMLEndElement(rd_ptr,strName);
		}

		// Sort out attributes...
		ProcessXmlValue(attribs,ptrKey,strData,bAdd,mapSubsts);
	}		
}

void OOCore::ReadXmlKey(const wchar_t*& rd_ptr, ObjectPtr<Registry::IRegistryKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
{
	std::map<string_t,string_t> attribs;
	bool bHasContent;
	string_t strName;
	Xml::ParseXMLElement(rd_ptr,strName,bHasContent,attribs);
	
	// Get out the namespaces
	std::map<string_t,string_t> namespaces2 = namespaces;
	Xml::ParseXMLNamespaces(attribs,namespaces2);

	// Check namespace...
	string_t strName2 = strName;
	string_t strNSpace;
	Xml::ParseXMLName(strName2,strNSpace,namespaces2);
	if (namespaces2[strNSpace] != xmlns)
    	OMEGA_THROW(L"Invalid schema");

	// Check what we have found...
	if (strName2 != L"key")
		OMEGA_THROW(L"key element expected");

	// Sort out attributes...
	ObjectPtr<Registry::IRegistryKey> ptrSubKey = ProcessXmlKeyAttribs(attribs,ptrKey,bAdd,mapSubsts);

	if (bHasContent)
	{
		// Skip any leading guff..
		string_t strGuff;
		Xml::ParseXMLCharData(rd_ptr,strGuff);	

		// Read key contents...
		while (ACE_OS::strncmp(rd_ptr,L"</",2) != 0)
		{
			ReadXmlKeyContents(rd_ptr,ptrSubKey,namespaces2,bAdd,mapSubsts);

			// Skip guff...
			Xml::ParseXMLCharData(rd_ptr,strGuff);
		} 

        Xml::ParseXMLEndElement(rd_ptr,strName);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Registry_AddXML,3,((in),const string_t&,strXML,(in),bool_t,bAdd,(in),const string_t&,strSubstitutions))
{
	// Parse the substitution map
	std::map<string_t,string_t> mapSubsts;
	const wchar_t* rd_ptr = strSubstitutions.c_str();

	for (;;)
	{
		// Skip the starting whitespace
		rd_ptr += ACE_OS::strspn(rd_ptr,L" ");

		// Find the next whitespace or terminator
		const wchar_t* p = ACE_OS::strpbrk(rd_ptr,L"=; ");
		if (!p)
			break;
		
		string_t strName = L"%" + string_t(rd_ptr,p - rd_ptr) + L"%";
		
		if (p[0] != L'=' && p[0] != L';')
			p += ACE_OS::strspn(p,L" ");

		if (p[0] != L'=')
			OMEGA_THROW(L"Invalid substitution string format");

		rd_ptr = p + 1;
		p = ACE_OS::strchr(rd_ptr,L';');

		string_t strVal;
		if (p)
			strVal = string_t(rd_ptr,p - rd_ptr);
		else
			strVal = rd_ptr;
		
		mapSubsts.insert(std::map<string_t,string_t>::value_type(strName,strVal));

		if (p)
			rd_ptr = p + 1;

		if (!p || p[0] == L'\0')
			break;	
	}
	
	// Parse the xml prolog
	rd_ptr = strXML.c_str();
	OOCore::Xml::ParseXMLProlog(rd_ptr);

	// Check we have a root node
	string_t strName;
	bool bHasContent;
	std::map<string_t,string_t> attribs;
	OOCore::Xml::ParseXMLElement(rd_ptr,strName,bHasContent,attribs);
	
	// Get out the namespaces
	std::map<string_t,string_t> namespaces;
	OOCore::Xml::ParseXMLNamespaces(attribs,namespaces);

	// Check namespace...
	string_t strName2 = strName;
	string_t strNSpace;
	OOCore::Xml::ParseXMLName(strName2,strNSpace,namespaces);
	if (namespaces[strNSpace] != OOCore::xmlns)
    	OMEGA_THROW(L"Invalid schema");

	// Check we have an root element
	if (strName2 != L"root")
		OMEGA_THROW(L"Invalid root element type");

	if (!bHasContent)
		OMEGA_THROW(L"Unexpected empty root element");
		
	// Skip the guff...
	string_t strGuff;
	OOCore::Xml::ParseXMLCharData(rd_ptr,strGuff);

	if (ACE_OS::strncmp(rd_ptr,L"</",2) == 0)
		OMEGA_THROW(L"Unexpected empty root element");

	ObjectPtr<Registry::IRegistryKey> ptrKey(L"\\");

	// Read keys...
	do
	{
		OOCore::ReadXmlKey(rd_ptr,ptrKey,namespaces,bAdd,mapSubsts);

		// Skip any guff...
		OOCore::Xml::ParseXMLCharData(rd_ptr,strGuff);

	} while (ACE_OS::strncmp(rd_ptr,L"</",2) != 0);
	
    // End of root element
	OOCore::Xml::ParseXMLEndElement(rd_ptr,strName);
}
