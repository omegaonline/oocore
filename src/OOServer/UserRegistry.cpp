///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer.h"
#include "./UserRegistry.h"
#include "./UserManager.h"
#include "./SpawnedProcess.h"

using namespace Omega;
using namespace Omega::Registry;
using namespace OTL;

namespace User
{
namespace Registry
{
	class BadNameException :
		public ExceptionImpl<IBadNameException>
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

		static void Validate(const string_t& strName, const string_t& strSource)
		{
			if (strName.IsEmpty() || strName.Left(1) == L"\\" ||
				strName.Find(L']') != string_t::npos ||
				strName.Find(L'[') != string_t::npos)
			{
				Throw(strName,strSource);
			}
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<BadNameException>* pRE = ObjectImpl<BadNameException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = string_t::Format(L"Invalid name for registry key or value: '%ls'.",name.c_str());
			throw pRE;
		}
	};

	class WrongValueTypeException :
		public ExceptionImpl<IWrongValueTypeException>
	{
	public:
		BEGIN_INTERFACE_MAP(WrongValueTypeException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IWrongValueTypeException>)
		END_INTERFACE_MAP()

		ValueType_t m_type;
		string_t    m_strValue;

	public:
		ValueType_t GetValueType()
		{
			return m_type;
		}

		string_t GetValueName()
		{
			return m_strValue;
		}

		static void Throw(string_t strValue, ValueType_t actual_type, const string_t& strSource)
		{
			ObjectImpl<WrongValueTypeException>* pRE = ObjectImpl<WrongValueTypeException>::CreateInstance();
			pRE->m_type = actual_type;
			pRE->m_strValue = strValue;
			pRE->m_strSource = strSource;
			
			string_t tp = L"Corrupt!";
			if (actual_type==String)
				tp = "String";
			else if (actual_type==UInt32)
				tp = "UInt32";
			else if (actual_type==Binary)
				tp = "Binary";

			pRE->m_strDesc = string_t::Format(L"Incorrect registry value type, actual value type is %ls.",tp.c_str());

			throw pRE;
		}
	};

	class NotFoundException :
		public ExceptionImpl<INotFoundException>
	{
	public:
		BEGIN_INTERFACE_MAP(NotFoundException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<INotFoundException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource, IException* pE = 0)
		{
			ObjectImpl<NotFoundException>* pRE = ObjectImpl<NotFoundException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = string_t::Format(L"'%ls' not found.",name.c_str());
			throw pRE;
		}
	};

	class AlreadyExistsException :
		public ExceptionImpl<IAlreadyExistsException>
	{
	public:
		BEGIN_INTERFACE_MAP(AlreadyExistsException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IAlreadyExistsException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetKeyName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<AlreadyExistsException>* pRE = ObjectImpl<AlreadyExistsException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = string_t::Format(L"Key '%ls' already exists.",name.c_str());
			throw pRE;
		}
	};

	class AccessDeniedException :
		public ExceptionImpl<IAccessDeniedException>
	{
	public:
		BEGIN_INTERFACE_MAP(AccessDeniedException)
			INTERFACE_ENTRY_CHAIN(ExceptionImpl<IAccessDeniedException>)
		END_INTERFACE_MAP()

		string_t m_strName;

	public:
		string_t GetKeyName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, const string_t& strSource)
		{
			ObjectImpl<AccessDeniedException>* pRE = ObjectImpl<AccessDeniedException>::CreateInstance();
			pRE->m_strName = name;
			pRE->m_strSource = strSource;
			pRE->m_strDesc = string_t::Format(L"Write attempt illegal for '%ls'.",name.c_str());
			throw pRE;
		}
	};
}
}

using namespace User;
using namespace User::Registry;

UserKey::UserKey() :
	m_pRegistry(0),
	m_pLock(0)
{
}

void UserKey::Init(ACE_Configuration_Heap* pRegistry, ACE_RW_Thread_Mutex* pLock, const Omega::string_t& strKeyName)
{
	m_pRegistry = pRegistry;
	m_pLock = pLock;
	m_strKeyName = strKeyName;
}

string_t UserKey::FullKeyPath(const string_t& strSub)
{
	string_t strRet = m_strKeyName + L"\\" + strSub;
	if (strRet.Left(1) == L"\\")
		return strRet.Mid(1);
	else
		return strRet;
}

ACE_Configuration_Section_Key UserKey::open_key()
{
	// Skip "Current User\\"
	string_t strKey = m_strKeyName.Mid(13);
	if (strKey.IsEmpty())
		return m_pRegistry->root_section();

	ACE_Configuration_Section_Key sub_key;
	if (m_pRegistry->open_section(m_pRegistry->root_section(),strKey.c_str(),0,sub_key) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(m_strKeyName,OMEGA_SOURCE_INFO);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return sub_key;
}

bool_t UserKey::IsSubKey(const string_t& strSubKey)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	ACE_Configuration_Section_Key sub_key;
	if (m_pRegistry->open_section(open_key(),strSubKey.c_str(),0,sub_key) == 0)
		return true;

	int err = ACE_OS::last_error();
	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");
		else
			OOSERVER_THROW_ERRNO(err);
	}
	return false;
}

bool_t UserKey::IsValue(const string_t& strName)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	ACE_Configuration_Heap::VALUETYPE vtype;
	if (m_pRegistry->find_value(open_key(),strName.c_str(),vtype) == 0)
		return true;

	int err = ACE_OS::last_error();
	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::IsValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}
	return false;
}

string_t UserKey::GetStringValue(const string_t& strName)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	ACE_WString strValue;
	if (m_pRegistry->get_string_value(open_key(),strName.c_str(),strValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetStringValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetStringValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return string_t(strValue.c_str());
}

uint32_t UserKey::GetUIntValue(const string_t& strName)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	u_int uValue = 0;
	if (m_pRegistry->get_integer_value(open_key(),strName.c_str(),uValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetUIntValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetUIntValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetUIntValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return static_cast<uint32_t>(uValue);
}

void UserKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	void* data = 0;
	size_t len = 0;
	if (m_pRegistry->get_binary_value(open_key(),strName.c_str(),data,len) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetUIntValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetUIntValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetUIntValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}

	if (cbLen && pBuffer)
	{
		cbLen = std::min(cbLen,static_cast<Omega::uint32_t>(len));
		if (data)
			memcpy(pBuffer,data,cbLen);
	}
	else
	{
		cbLen = static_cast<Omega::uint32_t>(len);
	}

	delete [] static_cast<char*>(data);
}

void UserKey::SetStringValue(const string_t& strName, const string_t& val)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetStringValue");

	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	ACE_WString strValue(val.c_str());
	if (m_pRegistry->set_string_value(open_key(),strName.c_str(),strValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetStringValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetStringValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::SetUIntValue(const string_t& strName, uint32_t val)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetUIntValue");

	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	if (m_pRegistry->set_integer_value(open_key(),strName.c_str(),val) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetUIntValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetUIntValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetUIntValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	if (m_pRegistry->set_binary_value(open_key(),strName.c_str(),val,cbLen) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetUIntValue");
			else
				NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetUIntValue");
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetUIntValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

ValueType_t UserKey::GetValueType(const string_t& strName)
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	ACE_Configuration_Heap::VALUETYPE vtype;
	if (m_pRegistry->find_value(open_key(),strName.c_str(),vtype) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetValueType");
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
		else
			OOSERVER_THROW_ERRNO(err);
	}

	switch (vtype)
	{
	case ACE_Configuration_Heap::STRING:
		return String;

	case ACE_Configuration_Heap::INTEGER:
		return UInt32;

	case ACE_Configuration_Heap::BINARY:
		return Binary;

	default:
		OOSERVER_THROW_ERRNO(EINVAL);
	}
}

IRegistryKey* UserKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	BadNameException::Validate(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	if (flags & IRegistryKey::FailIfThere)
	{
		// Check to see if the key already exists by opening it...
		ACE_Configuration_Section_Key sub_key;
		if (m_pRegistry->open_section(open_key(),strSubKey.c_str(),0,sub_key) == 0)
			AlreadyExistsException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::OpenSubKey");
	}

    int bCreate = (flags & IRegistryKey::Create) ? 1 : 0;

	ACE_Configuration_Section_Key sub_key;
	if (m_pRegistry->open_section(open_key(),strSubKey.c_str(),bCreate,sub_key) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::OpenSubKey");
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
		else
			OOSERVER_THROW_ERRNO(err);
	}

	// If we get here, then we have a new key!
	ObjectPtr<ObjectImpl<UserKey> > ptrSubKey = ObjectImpl<UserKey>::CreateInstancePtr();
	ptrSubKey->Init(m_pRegistry,m_pLock,m_strKeyName + L"\\" + strSubKey);
	return ptrSubKey.AddRefReturn();
}

Omega::IEnumString* UserKey::EnumSubKeys()
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	try
	{
		ACE_Configuration_Section_Key key = open_key();
		std::set<string_t> setSubKeys;
		for (int index=0;;++index)
		{
			ACE_WString strSubKey;
			int err = m_pRegistry->enumerate_sections(key,index,strSubKey);
			if (err == 0)
				setSubKeys.insert(strSubKey.c_str());
			else if (err == 1)
				break;
			else
				OOSERVER_THROW_ERRNO(err);
		}

		return EnumString::Create(setSubKeys.begin(),setSubKeys.end());
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

Omega::IEnumString* UserKey::EnumValues()
{
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	try
	{
		ACE_Configuration_Section_Key key = open_key();
		std::set<string_t> setValues;
		for (int index=0;;++index)
		{
			ACE_WString strValue;
			ACE_Configuration_Heap::VALUETYPE type;
			int err = m_pRegistry->enumerate_values(key,index,strValue,type);
			if (err == 0)
				setValues.insert(strValue.c_str());
			else if (err == 1)
				break;
			else
				OOSERVER_THROW_ERRNO(err);
		}

		return EnumString::Create(setValues.begin(),setValues.end());
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void UserKey::DeleteKey(const string_t& strSubKey)
{
	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	if (m_pRegistry->remove_section(open_key(),strSubKey.c_str(),1) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::DeleteKey");
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::DeleteValue(const string_t& strName)
{
	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,*m_pLock);

	if (m_pRegistry->remove_value(open_key(),strName.c_str()) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::DeleteValue");
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void RootKey::Init(Manager* pManager, const string_t& strKey)
{
	m_pManager = pManager;
	m_strKey = strKey;
}

string_t RootKey::FullKeyPath(const string_t& strSub)
{
	string_t strRet = m_strKey + L"\\" + strSub;
	if (strRet.Left(1) == L"\\")
		return strRet.Mid(1);
	else
		return strRet;
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::KeyExists);
	request.write_wstring(FullKeyPath(strSubKey).c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::Boolean bRes = false;
	if (!response.read_boolean(bRes))
		OOSERVER_THROW_LASTERROR();

	return bRes;
}

bool_t RootKey::IsValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::Octet value_type;
	if (err==0 && !response.read_octet(value_type))
		err = ACE_OS::last_error();

	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::IsValue");
		else if (err != 0)
			OOSERVER_THROW_ERRNO(err);
	}

	return (err==0);
}

ValueType_t RootKey::GetValueType(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetValueType");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetValueType");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::Octet value_type = 0;
	if (!response.read_octet(value_type))
		OOSERVER_THROW_LASTERROR();

	switch (value_type)
	{
	case ACE_Configuration_Heap::STRING:
		return String;

	case ACE_Configuration_Heap::INTEGER:
		return UInt32;

	case ACE_Configuration_Heap::BINARY:
		return Binary;

	default:
		OOSERVER_THROW_ERRNO(EINVAL);
	}
}

// Annoyingly this is missing from ACE...  A direct lift and translate of the ACE_CString version
static ACE_CDR::Boolean read_wstring(ACE_InputCDR& stream, ACE_WString& x)
{
	ACE_CDR::WChar *data = 0;
	if (stream.read_wstring(data))
	{
		x = data;
		delete [] data;
		return true;
	}

	x = L"";
	return stream.good_bit();
}

string_t RootKey::GetStringValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetStringValue);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetStringValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_WString strValue;
	if (!read_wstring(response,strValue))
		OOSERVER_THROW_LASTERROR();

	return strValue.c_str();
}

uint32_t RootKey::GetUIntValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetUInt32Value);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetUIntValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetUIntValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetUIntValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::ULong uValue = 0;
	if (!response.read_ulong(uValue))
		OOSERVER_THROW_LASTERROR();

	return uValue;
}

void RootKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetBinaryValue);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	request.write_ulong(cbLen);
	bool bNoDataBack = (cbLen == 0);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::GetBinaryValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::GetBinaryValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	if (!response.read_ulong(cbLen))
		OOSERVER_THROW_LASTERROR();

	if (!bNoDataBack && !response.read_octet_array(pBuffer,cbLen))
		OOSERVER_THROW_LASTERROR();
}

void RootKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetStringValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetStringValue);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	request.write_wstring(strValue.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetStringValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetStringValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::SetUIntValue(const string_t& strName, uint32_t uValue)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetUIntValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetUInt32Value);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	request.write_ulong(uValue);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetUIntValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetUIntValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetUIntValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetUIntValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	BadNameException::Validate(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetBinaryValue);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	request.write_ulong(cbLen);
	request.write_octet_array(val,cbLen);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(FullKeyPath(strName),GetValueType(strName),L"Omega::Registry::IRegistry::SetBinaryValue");
		else
			NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::SetBinaryValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

IRegistryKey* RootKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	BadNameException::Validate(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::KeyExists);
	request.write_wstring(FullKeyPath(strSubKey).c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
    response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::Boolean bRes;
	if (!response.read_boolean(bRes))
		OOSERVER_THROW_LASTERROR();

	if (!bRes)
	{
		if (!(flags & IRegistryKey::Create))
			NotFoundException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::OpenSubKey");

		// It doesn't yet exist, and we want to create it!
		request.reset();
		request << static_cast<Root::RootOpCode_t>(Root::CreateKey);
		request.write_wstring(FullKeyPath(strSubKey).c_str());
		if (!request.good_bit())
			OOSERVER_THROW_LASTERROR();

		response = m_pManager->sendrecv_root(request);

		response >> err;
		if (!response.good_bit())
			OOSERVER_THROW_LASTERROR();

		if (err==EACCES)
			AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::OpenSubKey");
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
		else if (err != 0)
			OOSERVER_THROW_ERRNO(err);
	}
	else if (flags & IRegistryKey::FailIfThere)
		AlreadyExistsException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::OpenSubKey");

	// By the time we get here then we have successfully created the key...
	ObjectPtr<ObjectImpl<RootKey> > ptrNew = ObjectImpl<RootKey>::CreateInstancePtr();

	ptrNew->Init(m_pManager,FullKeyPath(strSubKey));

	return ptrNew.AddRefReturn();
}

Omega::IEnumString* RootKey::EnumSubKeys()
{
	try
	{
		std::set<Omega::string_t> setStrings;
		EnumSubKeys(setStrings);

		return EnumString::Create(setStrings.begin(),setStrings.end());
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void RootKey::EnumSubKeys(std::set<Omega::string_t>& setStrings)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::EnumSubKeys);
	request.write_wstring(m_strKey.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::ULongLong count = 0;
	if (!response.read_ulonglong(count))
		OOSERVER_THROW_LASTERROR();

	for (ACE_CDR::ULongLong i=0;i<count;++i)
	{
		ACE_WString strName;
		if (!read_wstring(response,strName))
			OOSERVER_THROW_LASTERROR();

		setStrings.insert(strName.c_str());
	}
}

Omega::IEnumString* RootKey::EnumValues()
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::EnumValues);
	request.write_wstring(m_strKey.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	ACE_CDR::ULongLong count = 0;
	if (!response.read_ulonglong(count))
		OOSERVER_THROW_LASTERROR();

	try
	{
		std::set<Omega::string_t> setStrings;
		for (ACE_CDR::ULongLong i=0;i<count;++i)
		{
			ACE_WString strName;
			if (!read_wstring(response,strName))
				OOSERVER_THROW_LASTERROR();

			setStrings.insert(strName.c_str());
		}

		return EnumString::Create(setStrings.begin(),setStrings.end());
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void RootKey::DeleteKey(const string_t& strSubKey)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteKey);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strSubKey.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EACCES)
		AccessDeniedException::Throw(FullKeyPath(strSubKey),L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::DeleteValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteValue);
	request.write_wstring(m_strKey.c_str());
	request.write_wstring(strName.c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->sendrecv_root(request);

	int err = 0;
	response >> err;
	if (!response.good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(FullKeyPath(strName),L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void BaseKey::Init(Manager* pManager, bool bSandbox)
{
	if (open_registry(bSandbox) != 0)
		OOSERVER_THROW_LASTERROR();

	m_ptrRoot = ObjectImpl<RootKey>::CreateInstancePtr();
	m_ptrRoot->Init(pManager,L"");

	m_ptrUser = ObjectImpl<UserKey>::CreateInstancePtr();
	m_ptrUser->Init(&m_registry,&m_lock,L"Current User");
}

int BaseKey::open_registry(bool bSandbox)
{
#define OMEGA_REGISTRY_FILE L"user.regdb"

#if defined(ACE_WIN32)

	ACE_WString strRegistry = L"C:\\" OMEGA_REGISTRY_FILE;

	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr;
	if (bSandbox)
		hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	else
		hr = SHGetFolderPathW(0,CSIDL_LOCAL_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if SUCCEEDED(hr)
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (PathCombineW(szBuf2,szBuf,L"Omega Online"))
		{
			if (!PathFileExistsW(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					return ret;
			}

			if (PathCombineW(szBuf,szBuf2,OMEGA_REGISTRY_FILE))
				strRegistry = szBuf;
		}
	}

#else

	ACE_WString strDir;
	if (bSandbox)
		strDir = L"/var/lib/omegaonline";
	else
		strDir = ACE_Ascii_To_Wide((Root::SpawnedProcess::get_home_dir() + "/.omegaonline").c_str()).wchar_rep();

	if (ACE_OS::mkdir(strDir.c_str(),S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}

	ACE_WString strRegistry = strDir + L"/" OMEGA_REGISTRY_FILE;

#endif

	return m_registry.open(strRegistry.c_str(),(char*)0x40000000);
}

bool_t BaseKey::IsSubKey(const string_t& strSubKey)
{
	if (strSubKey == L"Current User")
		return true;
	else if (strSubKey.Left(13) == L"Current User\\")
		return m_ptrUser->IsSubKey(strSubKey.Mid(13));
	else
		return m_ptrRoot->IsSubKey(strSubKey);
}

bool_t BaseKey::IsValue(const string_t& strName)
{
	return m_ptrRoot->IsValue(strName);
}

string_t BaseKey::GetStringValue(const string_t& strName)
{
	return m_ptrRoot->GetStringValue(strName);
}

uint32_t BaseKey::GetUIntValue(const string_t& strName)
{
	return m_ptrRoot->GetUIntValue(strName);
}

void BaseKey::GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer)
{
	return m_ptrRoot->GetBinaryValue(strName,cbLen,pBuffer);
}

void BaseKey::SetStringValue(const string_t& strName, const string_t& val)
{
	return m_ptrRoot->SetStringValue(strName,val);
}

void BaseKey::SetUIntValue(const string_t& strName, uint32_t val)
{
	return m_ptrRoot->SetUIntValue(strName,val);
}

void BaseKey::SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val)
{
	return m_ptrRoot->SetBinaryValue(strName,cbLen,val);
}

ValueType_t BaseKey::GetValueType(const string_t& strName)
{
	return m_ptrRoot->GetValueType(strName);
}

IRegistryKey* BaseKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	if (strSubKey.IsEmpty() || strSubKey==L"\\")
	{
		AddRef();
		return this;
	}

    if (strSubKey==L"Current User")
		return m_ptrUser.AddRefReturn();
	else if (strSubKey.Left(13) == L"Current User\\")
		return m_ptrUser->OpenSubKey(strSubKey.Mid(13),flags);
	else
		return m_ptrRoot->OpenSubKey(strSubKey,flags);
}

Omega::IEnumString* BaseKey::EnumSubKeys()
{
	try
	{
		std::set<string_t>	setStrings;
		m_ptrRoot->EnumSubKeys(setStrings);

		setStrings.insert(L"Current User");

		return EnumString::Create(setStrings.begin(),setStrings.end());
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

Omega::IEnumString* BaseKey::EnumValues()
{
	return m_ptrRoot->EnumValues();
}

void BaseKey::DeleteKey(const string_t& strSubKey)
{
	if (strSubKey == L"Current User")
		OOSERVER_THROW_ERRNO(EACCES);

	m_ptrRoot->DeleteKey(strSubKey);
}

void BaseKey::DeleteValue(const string_t& strName)
{
	m_ptrRoot->DeleteValue(strName);
}
