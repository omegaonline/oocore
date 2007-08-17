#ifndef OOCORE_XML_H_INCLUDED_
#define OOCORE_XML_H_INCLUDED_

namespace OOCore
{
	namespace Xml
	{
		void ParseXMLProlog(const wchar_t*& rd_ptr);
		void ParseXMLName(Omega::string_t& strName, Omega::string_t& strNamespace, const std::map<Omega::string_t,Omega::string_t>& namespaces);
		void ParseXMLNamespaces(const std::map<Omega::string_t,Omega::string_t>& attribs, std::map<Omega::string_t,Omega::string_t>& namespaces);
		void ParseXMLElement(const wchar_t*& rd_ptr, Omega::string_t& strName, bool& bHasContents, std::map<Omega::string_t,Omega::string_t>& attribs);
		void ParseXMLCharData(const wchar_t*& rd_ptr, Omega::string_t& strData);
		void ParseXMLEndElement(const wchar_t*& rd_ptr, const Omega::string_t& strName);
	}
}

#endif // OOCORE_XML_H_INCLUDED_
