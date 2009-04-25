///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OORpc.h"

#include "HttpMsg.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(Rpc,OID_HttpOutputMsgMarshalFactory,"{B264BBE2-2DFF-45be-ADB4-1A80E6576A63}");
OMEGA_DEFINE_OID(Rpc,OID_HttpOutputMsg,"{C94CCB71-6345-40b1-B588-8EE91089B7AD}");

Rpc::HttpMsgBase::HttpMsgBase() :
	m_mb(0), m_bFirstItem(true)
{
	OMEGA_NEW(m_mb,ACE_Message_Block(ACE_OS::getpagesize()));
}

Rpc::HttpMsgBase::~HttpMsgBase()
{
	if (m_mb)
		m_mb->release();
}

bool Rpc::HttpMsgBase::IsWhitespace(char c)
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

void Rpc::HttpMsgBase::CheckChar(char c)
{
	if (m_mb->length() == 0)
		OMEGA_THROW(L"Invalid format!");

	char* pEnd = m_mb->rd_ptr();
	if (pEnd[0] != c)
		OMEGA_THROW(L"Invalid format!");

	m_mb->rd_ptr(1);
}

char Rpc::HttpMsgBase::PeekNextChar()
{
	if (m_mb->length() == 0)
		OMEGA_THROW(L"Invalid format!");

	return *m_mb->rd_ptr();
}

void Rpc::HttpMsgBase::SkipWhitespace()
{
	char* pEnd;
	for (pEnd = m_mb->rd_ptr();pEnd < m_mb->wr_ptr();++pEnd)
	{
		if (!IsWhitespace(*pEnd))
			break;
	}

	m_mb->rd_ptr(pEnd);
}

uint64_t Rpc::HttpMsgBase::ReadUInt()
{
	if (PeekNextChar() == '-')
		OMEGA_THROW(L"Illegal negative number!");

	if (PeekNextChar() == '0')
	{
		m_mb->rd_ptr(1);
		return 0;
	}

	uint64_t val = 0;
	char* pEnd = m_mb->rd_ptr();
	for (int c=0;pEnd < m_mb->wr_ptr();++pEnd)
	{
		if (pEnd[0] >= '0' && pEnd[0] <= '9')
		{
			val *= 10;
			val += (pEnd[0]-'0');
			++c;
		}
		else
		{
			if (c==0)
				OMEGA_THROW(L"Invalid format!");

			break;
		}
	}

	m_mb->rd_ptr(pEnd);

	return val;
}

int64_t Rpc::HttpMsgBase::ReadInt()
{
	bool bNeg = (PeekNextChar() == '-');
	if (bNeg)
		m_mb->rd_ptr(1);

	if (PeekNextChar() == '0')
	{
		m_mb->rd_ptr(1);
		return 0;
	}

	int64_t val = 0;
	char* pEnd = m_mb->rd_ptr();
	for (int c=0;pEnd < m_mb->wr_ptr();++pEnd)
	{
		if (pEnd[0] >= '0' && pEnd[0] <= '9')
		{
			val *= 10;
			val += (pEnd[0]-'0');
			++c;
		}
		else
		{
			if (c==0)
				OMEGA_THROW(L"Invalid format!");

			break;
		}
	}

	m_mb->rd_ptr(pEnd);

	return (bNeg ? -val : val);
}

float8_t Rpc::HttpMsgBase::ReadDouble()
{
	char* pEnd = 0;
	double val = ACE_OS::strtod(m_mb->rd_ptr(),&pEnd);

	m_mb->rd_ptr(pEnd);

	return val;
	
	//// Parse digits before the decimal point
	//float8_t val = static_cast<float8_t>(ReadInt());
	//
	//if (PeekNextChar() == '.')
	//{
	//	m_mb->rd_ptr(1);

	//	float8_t frac = 0.0;
	//	char* pEnd = m_mb->rd_ptr();
	//	for (int dp = 0;pEnd < m_mb->wr_ptr();++pEnd)
	//	{
	//		if (pEnd[0] >= '0' && pEnd[0] <= '9')
	//		{
	//			frac += (pEnd[0]-'0')/(10.0*(++dp));
	//		}
	//		else
	//		{
	//			if (dp == 0)
	//				OMEGA_THROW(L"Invalid format!");

	//			break;
	//		}
	//	}

	//	m_mb->rd_ptr(pEnd);

	//	val += frac;
	//}

	//// Check exponent
	//char next = PeekNextChar();
	//if (next == 'E' || next == 'e')
	//{
	//	m_mb->rd_ptr(1);

	//	val *= pow(10.0,static_cast<int>(ReadInt()));
	//}

	//return val;
}

string_t Rpc::HttpMsgBase::ReadString()
{
	string_t strText;
	bool bEscape = false;
	std::string strRawText;

	if (PeekNextChar() != '"')
		OMEGA_THROW(L"Invalid format!");

	char* pEnd = m_mb->rd_ptr()+1;
	for (;pEnd < m_mb->wr_ptr();++pEnd)
	{
		if (bEscape)
		{
			switch (pEnd[0])
			{
			case '"':
				strRawText += '"';
				break;

			case '\\':
				strRawText += '\\';
				break;

			case '/':
				strRawText += '/';
				break;

			case 'b':
				strRawText += '\b';
				break;

			case 'f':
				strRawText += '\f';
				break;

			case 'n':
				strRawText += '\n';
				break;

			case 'r':
				strRawText += '\r';
				break;

			case 't':
				strRawText += '\t';
				break;

			case 'u':
				{
					++pEnd;
					size_t val = 0;
					for (size_t i=4;i>0;--i)
					{
						if (pEnd >= m_mb->wr_ptr())
							OMEGA_THROW(L"Invalid format!");

						if (pEnd[0] >= '0' && pEnd[0] <= '9')
							val += ((pEnd[0]-'0') << ((i-1)*8));
						else if (pEnd[0] >= 'a' && pEnd[0] <= 'f')
							val += ((pEnd[0]-'a') << ((i-1)*8));
						else if (pEnd[0] >= 'A' && pEnd[0] <= 'F')
							val += ((pEnd[0]-'A') << ((i-1)*8));
						else
							OMEGA_THROW(L"Invalid unicode hex format!");

						++pEnd;
					}

					strText += string_t(strRawText.c_str(),true);
					strRawText.clear();

					wchar_t buf[2];
					buf[0] = static_cast<wchar_t>(val);
					buf[1] = L'\0';
					strText += buf;
				}
				break;

			default:
				OMEGA_THROW(L"Invalid escaped character!");
			}

			bEscape = false;
		}
		else
		{
			if (pEnd[0] == '"')
				break;
			else if (pEnd[0] == '\\')
				bEscape = true;
			else 
				strRawText += pEnd[0];
		}
	}

	if (pEnd[0] != '"' || bEscape)
		OMEGA_THROW(L"Invalid format!");

	if (!strRawText.empty())
		strText += string_t(strRawText.c_str(),true);

	m_mb->rd_ptr(pEnd+1);

	return strText;
}

void Rpc::HttpMsgBase::ParseName(const wchar_t* pszName)
{
	if (m_bFirstItem)
		m_bFirstItem = false;
	else
	{
		SkipWhitespace();
		CheckChar(',');
		SkipWhitespace();
	}

	// Check the string is correct
	if (ReadString() != pszName)
		OMEGA_THROW(L"Wrong name!");

	SkipWhitespace();
	CheckChar(':');
	SkipWhitespace();
}

size_t Rpc::HttpMsgBase::ReadBooleans(const wchar_t* pszName, size_t count, bool_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		// Read Boolean value
		if (m_mb->length() >= 5 && ACE_OS::strncmp(m_mb->rd_ptr(),"false",5)==0)
		{
			arr[i] = false;
			m_mb->rd_ptr(5);
		}
		else if (m_mb->length() >= 4 && ACE_OS::strncmp(m_mb->rd_ptr(),"true",4)==0)
		{
			arr[i] = true;
			m_mb->rd_ptr(4);
		}
		else
			OMEGA_THROW(L"Invalid boolean value!");

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadBytes(const wchar_t* pszName, size_t count, byte_t* arr)
{
	// Read the name
	ParseName(pszName);

	// Read as if a string
	if (*m_mb->rd_ptr() != '"')
	{
		if (count != 0)
		{
			// Just read as a UInt
			arr[0] = static_cast<byte_t>(ReadUInt());
		}
		return 1;
	}

	char* pStart = m_mb->rd_ptr()+1;
	char* pEnd = pStart;
	for (;pEnd < m_mb->wr_ptr();++pEnd)
	{
		if (pEnd[0] == '"')
			break;
	}

	if (pEnd >= m_mb->wr_ptr())
		OMEGA_THROW(L"Invalid format!");

	std::string strText(pStart,pEnd-pStart);

	// Bytes are recorded as a base64 encoded string
	size_t len = 0;
	ACE_Byte* pData = ACE_Base64::decode((const ACE_Byte*)(strText.c_str()),&len);
	if (pData)
	{
		memcpy(arr,pData,(len > count ? count : len));
		delete [] pData;
	}

	m_mb->rd_ptr(pEnd+1);
	return len;
}

size_t Rpc::HttpMsgBase::ReadInt16s(const wchar_t* pszName, size_t count, int16_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		int64_t v = ReadInt();
		if (v < SHRT_MIN || v > SHRT_MAX)
			OMEGA_THROW(L"Invalid 16 bit value!");

		arr[i] = static_cast<int16_t>(v);

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadUInt16s(const wchar_t* pszName, size_t count, uint16_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		uint64_t v = ReadUInt();
		if (v > USHRT_MAX)
			OMEGA_THROW(L"Invalid 16 bit value!");

		arr[i] = static_cast<uint16_t>(v);

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadInt32s(const wchar_t* pszName, size_t count, int32_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		int64_t v = ReadInt();
		if (v < LONG_MIN || v > LONG_MAX)
			OMEGA_THROW(L"Invalid 32 bit value!");

		arr[i] = static_cast<int32_t>(v);

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadUInt32s(const wchar_t* pszName, size_t count, uint32_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		uint64_t v = ReadUInt();
		if (v > ULONG_MAX)
			OMEGA_THROW(L"Invalid 32 bit value!");

		arr[i] = static_cast<uint32_t>(v);

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadInt64s(const wchar_t* pszName, size_t count, int64_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = ReadInt();

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadUInt64s(const wchar_t* pszName, size_t count, uint64_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = ReadUInt();

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadFloat4s(const wchar_t* pszName, size_t count, float4_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = static_cast<float4_t>(ReadDouble());
		
		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadFloat8s(const wchar_t* pszName, size_t count, float8_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = ReadDouble();
		
		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadStrings(const wchar_t* pszName, size_t count, string_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = ReadString();

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

size_t Rpc::HttpMsgBase::ReadGuids(const wchar_t* pszName, size_t count, guid_t* arr)
{
	// Read the name
	ParseName(pszName);

	bool bArray = (PeekNextChar() == '[');
	if (bArray)
	{
		// array
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	size_t i;
	for (i=0;i<count;++i)
	{
		if (i > 0)
		{
			// Read trailing ,
			CheckChar(',');
			SkipWhitespace();
		}

		arr[i] = guid_t::FromString(ReadString());

		SkipWhitespace();

		if (!bArray || PeekNextChar() == ']')
		{
			++i;
			break;
		}
	}

	return i;
}

void Rpc::HttpMsgBase::ReadStructStart(const wchar_t* pszName, const wchar_t*)
{
	// This will catch the read after write problem...
	if (m_mb->rd_ptr() == m_mb->base())
		m_bFirstItem = true;

	// We support nameless structs
	if (pszName)
		ParseName(pszName);
	else
	{
		if (m_bFirstItem)
			m_bFirstItem = false;
		else
		{
			SkipWhitespace();
			CheckChar(',');
		}
		SkipWhitespace();
	}

	CheckChar('{');
	SkipWhitespace();

	m_bFirstItem = true;
}

void Rpc::HttpMsgBase::ReadStructEnd(const wchar_t*)
{
	SkipWhitespace();
	CheckChar('}');
	SkipWhitespace();

	m_bFirstItem = false;
}

void Rpc::HttpMsgBase::GetContent(void* str)
{
	*static_cast<std::string*>(str) = std::string(m_mb->rd_ptr(),m_mb->length());
}

void Rpc::HttpMsgBase::SkipCont(char term, bool bNamed)
{
	m_mb->rd_ptr(1);

	for (;;)
	{
		SkipWhitespace();

		if (PeekNextChar() == term)
			break;

		SkipValue(bNamed);
	}

	m_mb->rd_ptr(1);
}

void Rpc::HttpMsgBase::SkipValue(bool bNamed)
{
	SkipWhitespace();

	if (PeekNextChar() == ',')
	{
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}

	if (bNamed)
	{
		ReadString();
		SkipWhitespace();
		CheckChar(':');
		SkipWhitespace();
	}

	char c = PeekNextChar();
	if (c == '{')
		SkipCont('}',true);
	else if (c == '[')
		SkipCont(']',false);
	else if (c == '"')
		ReadString();
	else
	{
		// Skip non-whitespace text
		char* pEnd;
		for (pEnd = m_mb->rd_ptr();pEnd < m_mb->wr_ptr();++pEnd)
		{
			if (IsWhitespace(*pEnd))
				break;
		}

		m_mb->rd_ptr(pEnd);
	}
}

void Rpc::HttpMsgBase::ReadSubObject(void* str)
{
	ReadStructStart(L"Content",0);

	const char* pStart = m_mb->rd_ptr();

	// SkipCont() assumes we are at the char before...
	m_mb->rd_ptr(const_cast<char*>(pStart-1));

	SkipCont('}',true);

	*static_cast<std::string*>(str) = std::string(pStart,m_mb->rd_ptr() - pStart - 1);
}

void Rpc::HttpMsgBase::MarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	OTL::ObjectPtr<IHttpMsg> ptrHM(pMessage);
	if (ptrHM)
	{
		ptrHM->WriteSubObject(m_mb->rd_ptr(),m_mb->total_length());
	}
	else
	{
		std::string strText(m_mb->rd_ptr(),m_mb->total_length());
		string_t strFullText(strText.c_str(),true);
		pMessage->WriteStrings(L"data",1,&strFullText);
	}
}

void Rpc::HttpMsgBase::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t&, Remoting::MarshalFlags_t)
{
	OTL::ObjectPtr<IHttpMsg> ptrHM(pMessage);
	if (ptrHM)
	{
		std::string str;
		ptrHM->ReadSubObject(&str);
	}
	else
	{
		string_t str;
		pMessage->ReadStrings(L"data",1,&str);
	}
}

void Rpc::HttpOutputMsg::grow_mb(size_t cbBytes)
{
	if (m_mb->space() >= cbBytes)
		return;

	if (m_mb->size(m_mb->size() + ACE_OS::getpagesize()) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteString(std::string str)
{
	// Escape illegal characters
	for (size_t pos = 0;pos < str.length();)
	{
		char c = str[pos];
		if (c == '"' || c == '\\')
		{
			str = str.substr(0,pos) + '\\' + str.substr(pos);
			pos+=2;
		}
		else if (c == '\b')
		{
			str = str.substr(0,pos) + "\\b" + str.substr(pos+1);
			pos+=2;
		}
		else if (c == '\f')
		{
			str = str.substr(0,pos) + "\\f" + str.substr(pos+1);
			pos+=2;
		}
		else if (c == '\n')
		{
			str = str.substr(0,pos) + "\\n" + str.substr(pos+1);
			pos+=2;
		}
		else if (c == '\r')
		{
			str = str.substr(0,pos) + "\\r" + str.substr(pos+1);
			pos+=2;
		}
		else if (c == '\t')
		{
			str = str.substr(0,pos) + "\\t" + str.substr(pos+1);
			pos+=2;
		}
		else if (c <= 0x09)
		{
			std::string strFirst = str.substr(0,pos);
			std::string strLast = str.substr(pos+1);
			str = strFirst + "\\u000";
			str += ('0' + c);
			str += strLast;
			pos+=6;
		}
		else if (c <= 0x0f)
		{
			std::string strFirst = str.substr(0,pos);
			std::string strLast = str.substr(pos+1);
			str = strFirst + "\\u000";
			str += ('a' + (c-0xa));
			str += strLast;
			pos+=6;
		}
		else if (c <= 0x19)
		{
			std::string strFirst = str.substr(0,pos);
			std::string strLast = str.substr(pos+1);
			str = strFirst + "\\u001";
			str += ('0' + (c-0x10));
			str += strLast;
			pos+=6;
		}
		else if (c <= 0x1f)
		{
			std::string strFirst = str.substr(0,pos);
			std::string strLast = str.substr(pos+1);
			str = strFirst + "\\u001";
			str += ('a' + (c-0x1a));
			str += strLast;
			pos+=6;
		}
		else
			++pos;
	}

	str = "\"" + str + "\"";

	grow_mb(str.length());
	if (m_mb->copy(str.c_str(),str.length()) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteName(const string_t& strName)
{
	if (m_bFirstItem)
		m_bFirstItem = false;
	else
	{
		grow_mb(2);
		if (m_mb->copy(", ",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	WriteString(strName.ToUTF8());

	grow_mb(2);
	if (m_mb->copy(": ",2) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteInt(const int64_t& v)
{
	char szBuf[64] = {0};
	int r = ACE_OS::snprintf(szBuf,63,"%lld",v);
	if (r == -1 || r >= 64)
		OMEGA_THROW(ACE_OS::last_error());

	grow_mb(r);
	if (m_mb->copy(szBuf,r) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteUInt(const uint64_t& v)
{
	char szBuf[64] = {0};
	int r = ACE_OS::snprintf(szBuf,63,"%llu",v);
	if (r == -1 || r >= 64)
		OMEGA_THROW(ACE_OS::last_error());

	grow_mb(r);
	if (m_mb->copy(szBuf,r) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteDouble(const double& v)
{
	char szBuf[64] = {0};
	int len = ACE_OS::snprintf(szBuf,63,"%#.50G",v);
	if (len == -1 || len >= 64)
		OMEGA_THROW(ACE_OS::last_error());

	// Remove trailing 0's
	--len;
	for (;len>1;--len)
	{
		if (szBuf[len] != '0' || szBuf[len-1] == '.')
			break;
		
		szBuf[len] = '\0';
	}
	
	grow_mb(len+1);
	if (m_mb->copy(szBuf,len+1) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteBooleans(const wchar_t* pszName, size_t count, const bool_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		if (arr[i])
		{
			grow_mb(4);
			if (m_mb->copy("true",4) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}
		else
		{
			grow_mb(5);
			if (m_mb->copy("false",5) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteBytes(const wchar_t* pszName, size_t count, const byte_t* arr)
{
	WriteName(pszName);

	if (count == 1)
	{
		// Just write as a UInt
		return WriteUInt(arr[0]);
	}

	// Bytes are recorded as a base64 encoded string
	std::string strText;
	size_t len = 0;
	ACE_Byte* pData = ACE_Base64::encode((const ACE_Byte*)arr,count,&len);
	if (pData)
	{
		strText = std::string((const char*)pData,len);
		delete [] pData;
	}

	strText = "\"" + strText + "\"";
	grow_mb(strText.length());
	if (m_mb->copy(strText.c_str(),strText.length()) == -1)
		OMEGA_THROW(ACE_OS::last_error());
}

void Rpc::HttpOutputMsg::WriteInt16s(const wchar_t* pszName, size_t count, const int16_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteUInt16s(const wchar_t* pszName, size_t count, const uint16_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteUInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteInt32s(const wchar_t* pszName, size_t count, const int32_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteUInt32s(const wchar_t* pszName, size_t count, const uint32_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteUInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteInt64s(const wchar_t* pszName, size_t count, const int64_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteUInt64s(const wchar_t* pszName, size_t count, const uint64_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteUInt(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteFloat4s(const wchar_t* pszName, size_t count, const float4_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteDouble(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteFloat8s(const wchar_t* pszName, size_t count, const float8_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteDouble(arr[i]);
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteStrings(const wchar_t* pszName, size_t count, const string_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		WriteString(arr[i].ToUTF8());
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteGuids(const wchar_t* pszName, size_t count, const guid_t* arr)
{
	WriteName(pszName);

	if (count == 0)
	{
		grow_mb(2);
		if (m_mb->copy("[]",2) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		return;
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("[",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	for (size_t i=0;i<count;++i)
	{
		if (i != 0)
		{
			grow_mb(2);
			if (m_mb->copy(", ",2) == -1)
				OMEGA_THROW(ACE_OS::last_error());
		}

		std::string strGuid = arr[i].ToString().ToUTF8();
		strGuid = "\"" + strGuid + "\"";

		grow_mb(strGuid.length());
		if (m_mb->copy(strGuid.c_str(),strGuid.length()) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}

	if (count != 1)
	{
		grow_mb(1);
		if (m_mb->copy("]",1) == -1)
			OMEGA_THROW(ACE_OS::last_error());
	}
}

void Rpc::HttpOutputMsg::WriteStructStart(const wchar_t* pszName, const wchar_t*)
{
	WriteName(pszName);

	grow_mb(2);
	if (m_mb->copy("{ ",2) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	m_bFirstItem = true;
}

void Rpc::HttpOutputMsg::WriteStructEnd(const wchar_t*)
{
	grow_mb(2);
	if (m_mb->copy(" }",2) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	m_bFirstItem = false;
}

void Rpc::HttpOutputMsg::WriteSubObject(const char* sz, size_t len)
{
	WriteStructStart(L"Content",0);

	grow_mb(len);
	if (m_mb->copy(sz,len) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	WriteStructEnd(0);
}

void Rpc::HttpInputMsg::init(const ACE_Message_Block* mb)
{
	m_mb->release();
	m_mb = mb->duplicate();
}

void Rpc::HttpInputMsg::init(const std::string& strText)
{
	if (m_mb->size(strText.length()) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	if (m_mb->copy(strText.c_str(),strText.length()) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	SkipWhitespace();
	if (PeekNextChar() == ',')
	{
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}
}

void Rpc::HttpInputMsg::skip_leader()
{
	SkipWhitespace();
	if (PeekNextChar() == ',')
	{
		m_mb->rd_ptr(1);
		SkipWhitespace();
	}
}

bool Rpc::HttpInputMsg::more_exists()
{
	return (m_mb->length() != 0);
}

void Rpc::HttpOutputMsgMarshalFactory::UnmarshalInterface(Remoting::IObjectManager*, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	std::string strUtf8;
	OTL::ObjectPtr<IHttpMsg> ptrHM(pMessage);
	if (ptrHM)
		ptrHM->ReadSubObject(&strUtf8);
	else
	{
		string_t str;
		pMessage->ReadStrings(L"data",1,&str);
		strUtf8 = str.ToUTF8();
	}

	ObjectPtr<ObjectImpl<HttpInputMsg> > ptrInput = ObjectImpl<HttpInputMsg>::CreateInstancePtr();
	ptrInput->init(strUtf8);
	pObject = ptrInput->QueryInterface(iid);
}
