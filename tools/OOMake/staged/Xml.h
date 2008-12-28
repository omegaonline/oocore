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

#ifndef OOMAKE_XML_H_INCLUDED_
#define OOMAKE_XML_H_INCLUDED_

#define XML_WHITESPACE "\x20\x09\x0D\x0A"

struct XMLElement
{
	std::string strName;
	std::string strNSpace;
	std::string strContent;
	std::map<std::string,std::string> mapAttribs;
	std::list<XMLElement> listElements;
};

void ParseXMLDOM(const std::string& strXML, XMLElement& element);

#endif // OOMAKE_XML_H_INCLUDED_
