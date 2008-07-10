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
	class BadNameException :
		public ExceptionImpl<Registry::IBadNameException>
	{
	public:
		BEGIN_INTERFACE_MAP(BadNameException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IBadNameException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<BadNameException>* pRE = ObjectImpl<BadNameException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = string_t::Format(L"Invalid name for registry key or value: '%ls'.",name.c_str());
			throw static_cast<Registry::IBadNameException*>(pRE);
		}
	};

	static ObjectPtr<Registry::IKey> GetRootKey();
	static void ReadXmlKey(const wchar_t*& rd_ptr, ObjectPtr<Registry::IKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	static void ReadXmlKeyContents(const wchar_t*& rd_ptr, ObjectPtr<Registry::IKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	static ObjectPtr<Registry::IKey> ProcessXmlKeyAttribs(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IKey> ptrKey, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	static int64_t ProcessXmlInteger(const wchar_t* p);
	static void ProcessXmlValue(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IKey> ptrKey, const string_t& strData, bool bAdd, const std::map<string_t,string_t>& mapSubsts);
	static string_t SubstituteNames(const string_t& strName, const std::map<string_t,string_t>& mapSubsts);

	static const wchar_t xmlns[] = L"http://www.omegaonline.org.uk/schemas/registry.xsd";
}

ObjectPtr<Registry::IKey> OOCore::GetRootKey()
{
	ObjectPtr<Registry::IKey> ptrKey;
	ptrKey.Attach(static_cast<Registry::IKey*>(OOCore::GetInterProcessService()->GetRegistry()));
	return ptrKey;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IKey*,IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IKey::OpenFlags_t,flags))
{
	if (key.Left(1) != L"\\")
		OOCore::BadNameException::Throw(key,L"Omega::Registry::OpenKey");

	if (key == L"\\")
		return OOCore::GetRootKey().AddRef();
	else
		return OOCore::GetRootKey()->OpenSubKey(key.Mid(1),flags);
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

ObjectPtr<Registry::IKey> OOCore::ProcessXmlKeyAttribs(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IKey> ptrKey, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
{
	std::map<string_t,string_t>::const_iterator i = attribs.find(L"name");
	if (i == attribs.end())
		OMEGA_THROW(L"Expected name attribute not found");

	string_t strName = SubstituteNames(i->second,mapSubsts);

	if (bAdd)
	{
		if (ptrKey)
			return ptrKey.OpenSubKey(strName,Registry::IKey::Create);
		else
			return ObjectPtr<Registry::IKey>(strName,Registry::IKey::Create);
	}
	else
	{
		bool bRemove = false;
		i = attribs.find(L"uninstall");
		if (i != attribs.end() && i->second==L"Remove")
			bRemove = true;

		if (!ptrKey && strName.Left(1) == L"\\")
		{
			ptrKey = ObjectPtr<Registry::IKey>(L"\\");
			strName = strName.Mid(1);
		}

		if (ptrKey && ptrKey->IsSubKey(strName))
		{
			if (bRemove)
				ptrKey->DeleteKey(strName);
			else
				return ptrKey.OpenSubKey(strName,Registry::IKey::OpenExisting);
		}			
	}

	return 0;
}

int64_t OOCore::ProcessXmlInteger(const wchar_t* p)
{
	p += ACE_OS::strspn(p,L" ");

	bool bNeg = false;
	int base = 10;
	if (*p==L'-')
	{
		bNeg = true;
		++p;
	}
	else if (*p==L'+')
	{
		++p;
	}
	else if (p[0]==L'0' && p[1]==L'x')
	{
		p += 2;
		base = 16;
	}
	
	int64_t val = 0;
	for (;;++p)
	{
		int v = 0;
		if (*p >= L'0' && *p <= L'9')
			v = (*p - L'0');
		else if (base == 16)
		{
			if (*p >= L'A' && *p <= L'F')
				v = (*p - L'A' + 0xA);
			else if (*p >= L'a' && *p <= L'f')
				v = (*p - L'a' + 0xA);
			else
				break;
		}
		else
			break;

		val *= base;
		val += v;
	}

	if (bNeg)
		val = -val;

	return val;
}

void OOCore::ProcessXmlValue(const std::map<string_t,string_t>& attribs, ObjectPtr<Registry::IKey> ptrKey, const string_t& strData, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
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
		else if (strType == L"Integer")
			ptrKey->SetIntegerValue(strName,ProcessXmlInteger(strData2.c_str()));
		else if (strType == L"Binary")
			OMEGA_THROW(L"Binary values are not supported");
		else
			OMEGA_THROW(L"Invalid value type");
	}
}

void OOCore::ReadXmlKeyContents(const wchar_t*& rd_ptr, ObjectPtr<Registry::IKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
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
		ObjectPtr<Registry::IKey> ptrSubKey = ProcessXmlKeyAttribs(attribs,ptrKey,bAdd,mapSubsts);

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

void OOCore::ReadXmlKey(const wchar_t*& rd_ptr, ObjectPtr<Registry::IKey> ptrKey, const std::map<string_t,string_t>& namespaces, bool bAdd, const std::map<string_t,string_t>& mapSubsts)
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
	ObjectPtr<Registry::IKey> ptrSubKey = ProcessXmlKeyAttribs(attribs,ptrKey,bAdd,mapSubsts);

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

	// Read keys...
	do
	{
		OOCore::ReadXmlKey(rd_ptr,0,namespaces,bAdd,mapSubsts);

		// Skip any guff...
		OOCore::Xml::ParseXMLCharData(rd_ptr,strGuff);

	} while (ACE_OS::strncmp(rd_ptr,L"</",2) != 0);

	// End of root element
	OOCore::Xml::ParseXMLEndElement(rd_ptr,strName);
}
