///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOXML, the Omega Online XML library.
//
// OOXML is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOXML is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOXML.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#if !defined(OOXML_XML_H_INCLUDED_)
#define OOXML_XML_H_INCLUDED_

#include <Omega/IO.h>

namespace Omega
{
	namespace XML
	{
		interface ITokenizer : public IObject
		{
			enum Token
			{
				
			};
			typedef uint16_t Token_t;
			
			virtual void SetInputString(const string_t& strText) = 0;
			virtual void SetInputStream(IO::InputStream* pInStream) = 0;
			
			virtual bool GetToken(Token_t& token, string_t& value) = 0;
		};
		
		interface IParseException : public IException
		{
			virtual uint64_t GetLine() = 0;
			virtual uint32_t GetColumn() = 0;
		};
	}
}

#endif // OOXML_XML_H_INCLUDED_
