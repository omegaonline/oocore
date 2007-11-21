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
