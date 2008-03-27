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

#include "./Xml.h"

using namespace Omega;

namespace OOCore
{
	namespace Xml
	{
		void XMLSplitAttr(const string_t& strAttr, std::map<string_t,string_t>& attribs);
		void XMLSplitAttrs(const wchar_t*& rd_ptr, const wchar_t* pszTerm, size_t cbTerm, std::map<string_t,string_t>& attribs);		
	}
}

#define XML_WHITESPACE L"\x20\x09\x0D\x0A"

void OOCore::Xml::XMLSplitAttr(const string_t& strAttr, std::map<string_t,string_t>& attribs)
{
	// Attribute    ::=    Name Eq AttValue

	size_t pos = strAttr.Find(L'=');
	if (pos == string_t::npos)
		OMEGA_THROW(string_t::Format(L"%ls is not an XML attribute",strAttr.c_str()));

	string_t strVal;
	if (strAttr[pos+1] == L'"' || strAttr[pos+1] == L'\'')
	{
		size_t pos2 = strAttr.Find(strAttr[pos+1],pos+2);
		if (pos2 == string_t::npos)
			OMEGA_THROW(string_t::Format(L"Unmatched quote in attribute %ls",strAttr.c_str()));

		strVal = strAttr.Mid(pos+2,pos2-pos-2);
	}
	else
		OMEGA_THROW(string_t::Format(L"Unquoted attribute value %ls",strAttr.c_str()));

	attribs.insert(std::map<string_t,string_t>::value_type(strAttr.Left(pos),strVal));
}

void OOCore::Xml::XMLSplitAttrs(const wchar_t*& rd_ptr, const wchar_t* pszTerm, size_t cbTerm, std::map<string_t,string_t>& attribs)
{
	wchar_t szTerm[] = L"\0" XML_WHITESPACE;
	szTerm[0] = *pszTerm;

	// Skip the starting whitespace
	rd_ptr += ACE_OS::strspn(rd_ptr,XML_WHITESPACE);
	
	for (;;)
	{
		// Find the next whitespace or terminator
		const wchar_t* p = ACE_OS::strpbrk(rd_ptr,szTerm);
		if (!p)
			OMEGA_THROW(string_t::Format(L"Incomplete XML text %ls",string_t(rd_ptr,25).c_str()));

		if (p != rd_ptr)
			XMLSplitAttr(string_t(rd_ptr,p - rd_ptr),attribs);

		rd_ptr = p;
		
		// Skip the rest of the whitespace
		rd_ptr += ACE_OS::strspn(rd_ptr,XML_WHITESPACE);

		if (ACE_OS::strncmp(rd_ptr,pszTerm,cbTerm)==0)
		{
			rd_ptr += cbTerm;
			break;
		}		
	}
}

void OOCore::Xml::ParseXMLProlog(const wchar_t*& rd_ptr)
{
	// prolog    ::=    XMLDecl? Misc* (doctypedecl Misc*)? 

	// Check for prolog
	const wchar_t* p = ACE_OS::strstr(rd_ptr,L"<?xml");
	if (!p)
		OMEGA_THROW(L"Text is not XML");

	rd_ptr = p + 5;

	std::map<string_t,string_t> attribs;
	XMLSplitAttrs(rd_ptr,L"?>",2,attribs);
		
	std::map<string_t,string_t>::iterator i = attribs.find(L"version");
	if (i==attribs.end() || i->second != L"1.0")
		OMEGA_THROW(L"Invalid or missing XML version attribute");

	// Now we need to skip the extra stuff...
	for (;;)
	{	
		bool bFoundOne = false;

		// Skip comments
		p = ACE_OS::strstr(rd_ptr,L"<!--");
		if (p)
		{
			attribs.clear();
			XMLSplitAttrs(rd_ptr,L"-->",3,attribs);
				
			bFoundOne = true;
			rd_ptr = p;
		}

		// Skip processing instructions
		p = ACE_OS::strstr(rd_ptr,L"<?");
		if (p)
		{
			attribs.clear();
			XMLSplitAttrs(rd_ptr,L"?>",2,attribs);

			bFoundOne = true;
			rd_ptr = p;
		}

		// Skip the starting whitespace
		size_t ws = ACE_OS::strspn(rd_ptr,XML_WHITESPACE);
		if (ws)
		{
			bFoundOne = true;
			rd_ptr += ws;
		}

		if (!bFoundOne)
			break;
	}
}

void OOCore::Xml::ParseXMLElement(const wchar_t*& rd_ptr, string_t& strName, bool& bHasContents, std::map<string_t,string_t>& attribs)
{
	const wchar_t* p = rd_ptr;
	if (p[0] != L'<')
		OMEGA_THROW(L"ParseXMLElement called inappropriately");

	++p;
	size_t len = ACE_OS::strcspn(p,L"/>" XML_WHITESPACE);
	if (!len)
		OMEGA_THROW(string_t::Format(L"Unterminated XML element: %ls",string_t(rd_ptr,25).c_str()));

	strName = string_t(p,len);
	rd_ptr += len+1;
	
	// Skip the starting whitespace
	rd_ptr += ACE_OS::strspn(rd_ptr,XML_WHITESPACE);
	
	bHasContents = false;
	for (;;)
	{
		// Find the next whitespace or terminator
		p = rd_ptr;
		for (;;)
		{
			p = ACE_OS::strpbrk(p,L"'\"/>" XML_WHITESPACE);
			if (!p)
				OMEGA_THROW(string_t::Format(L"Unterminated XML element: %ls",string_t(rd_ptr,25).c_str()));

			if (p[0]==L'"' || p[0]==L'\'')
			{
				p = ACE_OS::strchr(p+1,p[0]);
				if (!p)
					OMEGA_THROW(string_t::Format(L"Mismatched quote: %ls",string_t(rd_ptr,25).c_str()));
				++p;
				continue;
			}
			else if (p[0]==L'/' && p[1]!=L'>')
			{
				++p;
				continue;
			}
			else				
				break;
		}

		if (p != rd_ptr)
			XMLSplitAttr(string_t(rd_ptr,p - rd_ptr),attribs);
		
		// Skip the rest of the whitespace
		p += ACE_OS::strspn(p,XML_WHITESPACE);

		if (p[0] == L'/' && p[1] == L'>')
		{
			rd_ptr = p + 2;
			break;
		}

		if (p[0] == L'>')
		{
			rd_ptr = p + 1;
			bHasContents = true;
			break;
		}
		
		rd_ptr = p;
	}
}

void OOCore::Xml::ParseXMLCharData(const wchar_t*& rd_ptr, string_t& strData)
{
	for (;;)
	{
		// Find the next whitespace or terminator
		const wchar_t* p = ACE_OS::strpbrk(rd_ptr,L"<&");
		if (!p)
		{
			strData += rd_ptr;
			break;
		}
		else if (p[0] == L'&')
		{
			if (ACE_OS::strncmp(p,L"&amp;",5)==0)
			{
				strData += string_t(rd_ptr,p - rd_ptr);
				strData += L"&";
				rd_ptr = p + 5;
			}
			else if (ACE_OS::strncmp(p,L"&lt;",4)==0)
			{
				strData += string_t(rd_ptr,p - rd_ptr);
				strData += L"<";
				rd_ptr = p + 4;
			}
			else if (ACE_OS::strncmp(p,L"&gt;",4)==0)
			{
				strData += string_t(rd_ptr,p - rd_ptr);
				strData += L">";
				rd_ptr = p + 4;
			}
			else if (ACE_OS::strncmp(p,L"&apos;",6)==0)
			{
				strData += string_t(rd_ptr,p - rd_ptr);
				strData += L"'";
				rd_ptr = p + 6;
			}
			else if (ACE_OS::strncmp(p,L"&quot;",6)==0)
			{
				strData += string_t(rd_ptr,p - rd_ptr);
				strData += L"\"";
				rd_ptr = p + 6;
			}
			else
			{
				// We don't support all this advanced stuff!
				OMEGA_THROW(L"Custom xml escaping isn't supported");
			}
		}
		else 
		{
			if (p == rd_ptr)
				break;

			strData += string_t(rd_ptr,p - rd_ptr);
			rd_ptr = p;

			// Now we need to skip the extra crap...
			std::map<string_t,string_t> attribs;
			bool bFoundOne = false;

			// Skip comments
			if (ACE_OS::strncmp(rd_ptr,L"<!--",4)==0)
			{
				attribs.clear();
				XMLSplitAttrs(rd_ptr,L"-->",3,attribs);
				
				bFoundOne = true;
			}

			// Skip processing instructions
			if (ACE_OS::strncmp(rd_ptr,L"<?",2)==0)
			{
				attribs.clear();
				XMLSplitAttrs(rd_ptr,L"?>",2,attribs);
					
				bFoundOne = true;
			}

			// Skip CDATA
			if (ACE_OS::strncmp(rd_ptr,L"<![CDATA[",9)==0)
			{
				attribs.clear();
				XMLSplitAttrs(rd_ptr,L"]]>",3,attribs);
					
				bFoundOne = true;
			}
						
			if (!bFoundOne)
				break;
		}
	}
}

void OOCore::Xml::ParseXMLEndElement(const wchar_t*& rd_ptr, const string_t& strName)
{
	const wchar_t* p = rd_ptr;

	if (ACE_OS::strncmp(p,L"</",2)!=0 || ACE_OS::strncmp(p+2,strName.c_str(),strName.Length())!=0)
		OMEGA_THROW(string_t::Format(L"Invalid element end tag: %ls",string_t(rd_ptr,25).c_str()));

	// Skip whistepace
	p += strName.Length()+2;
	p += ACE_OS::strspn(p,XML_WHITESPACE);
	
	if (p[0] != L'>')
		OMEGA_THROW(string_t::Format(L"Invalid element end tag: %ls",string_t(rd_ptr,25).c_str()));
	
	rd_ptr = p+1;
}

void OOCore::Xml::ParseXMLNamespaces(const std::map<string_t,string_t>& attribs, std::map<string_t,string_t>& namespaces)
{
	for (std::map<string_t,string_t>::const_iterator i=attribs.begin();i!=attribs.end();++i)
	{
		if (i->first.Length()>6 && i->first.Left(6)==L"xmlns:")
		{
			namespaces.insert(std::map<string_t,string_t>::value_type(i->first.Mid(6),i->second));
		}
		else if (i->first==L"xmlns")
		{
			if (namespaces.find(L"") != namespaces.end())
				OMEGA_THROW(L"Duplicate undecorated namespace attribute!");

			namespaces.insert(std::map<string_t,string_t>::value_type(L"",i->second));
		}
	}
}

void OOCore::Xml::ParseXMLName(string_t& strName, string_t& strNamespace, const std::map<string_t,string_t>& namespaces)
{
	size_t pos = strName.Find(L':');
	if (pos != string_t::npos)
	{
		strNamespace = strName.Left(pos);
		if (namespaces.find(strNamespace) == namespaces.end())
			OMEGA_THROW(string_t::Format(L"Unrecognised namespace qualification: %ls",strName.c_str()));

		strName = strName.Mid(pos+1);
	}
}
