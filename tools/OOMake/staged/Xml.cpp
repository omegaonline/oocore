///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOMake, the Omega Online Make application.
//
// OOMake is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOMake is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOMake.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "./Main.h"

static void XMLSplitAttr(const std::string& strAttr, std::map<std::string,std::string>& attribs)
{
	// Attribute    ::=    Name Eq AttValue

	size_t pos = strAttr.find('=');
	if (pos == std::string::npos)
	{
		std::ostringstream os;
		os << "Error: " << strAttr << " is not an XML attribute";
		throw os.str();
	}

	std::string strVal;
	if (strAttr[pos+1] == '"' || strAttr[pos+1] == '\'')
	{
		size_t pos2 = strAttr.find(strAttr[pos+1],pos+2);
		if (pos2 == std::string::npos)
		{
			std::ostringstream os;
			os << "Error: Unmatched quote in attribute " << strAttr;
			throw os.str();
		}

		strVal = strAttr.substr(pos+2,pos2-pos-2);
	}
	else
	{
		std::ostringstream os;
		os << "Error: Unquoted attribute value " << strAttr;
		throw os.str();
	}

	attribs.insert(std::map<std::string,std::string>::value_type(strAttr.substr(0,pos),strVal));
}

static void XMLSplitAttrs(const char*& rd_ptr, const char* pszTerm, size_t cbTerm, std::map<std::string,std::string>& attribs)
{
	char szTerm[] = "\0" XML_WHITESPACE;
	szTerm[0] = *pszTerm;

	// Skip the starting whitespace
	rd_ptr += strspn(rd_ptr,XML_WHITESPACE);
	
	for (;;)
	{
		// Find the next whitespace or terminator
		const char* p = strpbrk(rd_ptr,szTerm);
		if (!p)
		{
			std::ostringstream os;
			os << "Error: Incomplete XML text " << std::string(rd_ptr,25);
			throw os.str();
		}

		if (p != rd_ptr)
			XMLSplitAttr(std::string(rd_ptr,p - rd_ptr),attribs);

		rd_ptr = p;
		
		// Skip the rest of the whitespace
		rd_ptr += strspn(rd_ptr,XML_WHITESPACE);

		if (strncmp(rd_ptr,pszTerm,cbTerm)==0)
		{
			rd_ptr += cbTerm;
			break;
		}		
	}
}

static void ParseXMLElement(const char*& rd_ptr, std::string& strName, bool& bHasContents, std::map<std::string,std::string>& attribs)
{
	const char* p = rd_ptr;
	if (p[0] != '<')
		throw std::string("Error: ParseXMLElement called inappropriately");

	++p;
	size_t len = strcspn(p,"/>" XML_WHITESPACE);
	if (!len)
	{
		std::ostringstream os;
		os << "Error: Unterminated XML element: " << std::string(rd_ptr,25);
		throw os.str();
	}

	strName = std::string(p,len);
	rd_ptr += len+1;
	
	// Skip the starting whitespace
	rd_ptr += strspn(rd_ptr,XML_WHITESPACE);
	
	bHasContents = false;
	for (;;)
	{
		// Find the next whitespace or terminator
		p = rd_ptr;
		for (;;)
		{
			p = strpbrk(p,"'\"/>" XML_WHITESPACE);
			if (!p)
			{
				std::ostringstream os;
				os << "Error: Unterminated XML element: " << std::string(rd_ptr,25);
				throw os.str();
			}

			if (p[0]=='"' || p[0]=='\'')
			{
				p = strchr(p+1,p[0]);
				if (!p)
				{
					std::ostringstream os;
					os << "Error: Mismatched quote: " << std::string(rd_ptr,25);
					throw os.str();
				}

				++p;
				continue;
			}
			else if (p[0]=='/' && p[1]!='>')
			{
				++p;
				continue;
			}
			else				
				break;
		}

		if (p != rd_ptr)
			XMLSplitAttr(std::string(rd_ptr,p - rd_ptr),attribs);
		
		// Skip the rest of the whitespace
		p += strspn(p,XML_WHITESPACE);

		if (p[0] == '/' && p[1] == '>')
		{
			rd_ptr = p + 2;
			break;
		}

		if (p[0] == '>')
		{
			rd_ptr = p + 1;
			bHasContents = true;
			break;
		}
		
		rd_ptr = p;
	}
}

static void ParseXMLCharData(const char*& rd_ptr, std::string& strData)
{
	for (;;)
	{
		// Find the next whitespace or terminator
		const char* p = strpbrk(rd_ptr,"<&");
		if (!p)
		{
			strData += rd_ptr;
			break;
		}
		else if (p[0] == '&')
		{
			if (strncmp(p,"&amp;",5)==0)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				strData += "&";
				rd_ptr = p + 5;
			}
			else if (strncmp(p,"&lt;",4)==0)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				strData += "<";
				rd_ptr = p + 4;
			}
			else if (strncmp(p,"&gt;",4)==0)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				strData += ">";
				rd_ptr = p + 4;
			}
			else if (strncmp(p,"&apos;",6)==0)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				strData += "'";
				rd_ptr = p + 6;
			}
			else if (strncmp(p,"&quot;",6)==0)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				strData += "\"";
				rd_ptr = p + 6;
			}
			else
			{
				// We don't support all this advanced stuff!
				throw std::string("Error: Custom xml escaping isn't supported");
			}
		}
		else 
		{
			if (p != rd_ptr)
			{
				strData += std::string(rd_ptr,p - rd_ptr);
				rd_ptr = p;
			}

			// Now we need to skip the extra crap...
			std::map<std::string,std::string> attribs;
			bool bFoundOne = false;

			// Skip comments
			if (strncmp(rd_ptr,"<!--",4)==0)
			{
				p = strstr(rd_ptr+4,"-->");
				if (!p)
				{
					std::ostringstream os;
					os << "Error: Unmatched comment open: " << std::string(rd_ptr,25);
					throw os.str();
				}

				rd_ptr = p + 3;				
				bFoundOne = true;
			}

			// Skip processing instructions
			if (strncmp(rd_ptr,"<?",2)==0)
			{
				p = strstr(rd_ptr+2,"?>");
				if (!p)
				{
					std::ostringstream os;
					os << "Error: Unmatched processing instruction open: " << std::string(rd_ptr,25);
					throw os.str();
				}

				rd_ptr = p + 2;
				bFoundOne = true;
			}

			// Skip CDATA
			if (strncmp(rd_ptr,"<![CDATA[",9)==0)
			{
				p = strstr(rd_ptr+9,"]]>");
				if (!p)
				{
					std::ostringstream os;
					os << "Error: Unmatched CDATA open: " << std::string(rd_ptr,25);
					throw os.str();
				}

				rd_ptr = p + 3;
				bFoundOne = true;
			}
						
			if (!bFoundOne)
				break;
		}
	}
}

static void ParseXMLEndElement(const char*& rd_ptr, const std::string& strName)
{
	const char* p = rd_ptr;

	if (strncmp(p,"</",2)!=0 || strncmp(p+2,strName.c_str(),strName.length())!=0)
	{
		std::ostringstream os;
		os << "Error: Invalid element end tag: " << std::string(rd_ptr,25);
		throw os.str();
	}

	// Skip whistepace
	p += strName.length()+2;
	p += strspn(p,XML_WHITESPACE);
	
	if (p[0] != '>')
	{
		std::ostringstream os;
		os << "Error: Invalid element end tag: " << std::string(rd_ptr,25);
		throw os.str();
	}
	
	rd_ptr = p+1;
}

static void ParseXMLNamespaces(const std::map<std::string,std::string>& attribs, std::map<std::string,std::string>& namespaces)
{
	for (std::map<std::string,std::string>::const_iterator i=attribs.begin();i!=attribs.end();++i)
	{
		if (i->first.length()>6 && i->first.substr(0,6)=="xmlns:")
		{
			namespaces.insert(std::map<std::string,std::string>::value_type(i->first.substr(6),i->second));
		}
		else if (i->first=="xmlns")
		{
			if (namespaces.find("") != namespaces.end())
				throw std::string("Error: Duplicate undecorated namespace attribute!");

			namespaces.insert(std::map<std::string,std::string>::value_type("",i->second));
		}
	}
}

static void ParseXMLName(std::string& strName, std::string& strNamespace, const std::map<std::string,std::string>& namespaces)
{
	size_t pos = strName.find(':');
	if (pos != std::string::npos)
	{
		strNamespace = strName.substr(0,pos);
		if (namespaces.find(strNamespace) == namespaces.end())
		{
			std::ostringstream os;
			os << "Error: Unrecognised namespace qualification: " << strName;
			throw os.str();
		}

		strName = strName.substr(pos+1);
	}
}

static void ParseXMLDOMElement(const char*& rd_ptr, XMLElement& element, const std::map<std::string,std::string>& namespaces)
{
	// Parse attribs
	bool bHasContent;
	ParseXMLElement(rd_ptr,element.strName,bHasContent,element.mapAttribs);
	
	// Get out the namespaces
	std::map<std::string,std::string> namespaces2 = namespaces;
	ParseXMLNamespaces(element.mapAttribs,namespaces2);

	// Check namespace...
	std::string strNSpace;
	ParseXMLName(element.strName,strNSpace,namespaces2);
	std::map<std::string,std::string>::const_iterator i = namespaces2.find(strNSpace);
	if (i != namespaces2.end())
		element.strNSpace = i->second;
	
	if (bHasContent)
	{
		std::string strCharData;
		ParseXMLCharData(rd_ptr,strCharData);	
		element.strContent += strCharData;

		// Read key contents...
		while (strncmp(rd_ptr,"</",2) != 0)
		{
			// Parse the sub element
			XMLElement sub_element;
			ParseXMLDOMElement(rd_ptr,sub_element,namespaces2);
			element.listElements.push_back(sub_element);

			// Skip guff...
			ParseXMLCharData(rd_ptr,strCharData);
			element.strContent += strCharData;
		} 

		ParseXMLEndElement(rd_ptr,element.strName);
	}
}

static void ParseXMLProlog(const char*& rd_ptr)
{
	// prolog    ::=    XMLDecl? Misc* (doctypedecl Misc*)? 

	// Check for prolog
	const char* p = strstr(rd_ptr,"<?xml");
	if (!p)
		throw std::string("Error: Text is not XML");

	rd_ptr = p + 5;

	std::map<std::string,std::string> attribs;
	XMLSplitAttrs(rd_ptr,"?>",2,attribs);
		
	std::map<std::string,std::string>::iterator i = attribs.find("version");
	if (i==attribs.end() || i->second != "1.0")
		throw std::string("Error: Invalid or missing XML version attribute");

	// Now we need to skip the extra stuff...
	std::string strCharData;
	ParseXMLCharData(rd_ptr,strCharData);	
}

void ParseXMLDOM(const std::string& strXML, XMLElement& element)
{
	// Parse the xml prolog
	const char* rd_ptr = strXML.c_str();
	ParseXMLProlog(rd_ptr);

	std::map<std::string,std::string> namespaces;
	ParseXMLDOMElement(rd_ptr,element,namespaces);
}
