///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrLite, the Omega Online Standalone plugin.
//
// OOSvrLite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrLite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrLite.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Lite.h"

#if defined(_WIN32)
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include <OTL/Exception.h>
#include <OTL/Registry.h>

#include "IPSLite.h"
#include "RegistryHive.h"

using namespace Omega;
using namespace Omega::Registry;
using namespace OTL;

#include "RegistryCmn.h"

namespace
{
	class RootKey;

	class HiveKey :
		public ObjectBase,
		public IKey
	{
	public:
		void Init(::Registry::Hive* pHive, const Omega::string_t& strKey, const Omega::int64_t& key);

		BEGIN_INTERFACE_MAP(HiveKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		::Registry::Hive* m_pHive;
		Omega::string_t   m_strKey;
		Omega::int64_t    m_key;

		int GetValueType_i(const string_t& strName, ValueType_t& vtype);

	// IRegistry members
	public:
		bool_t IsSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		string_t GetStringValue(const string_t& strName);
		int64_t GetIntegerValue(const string_t& strName);
		void GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer);
		void SetStringValue(const string_t& strName, const string_t& strValue);
		void SetIntegerValue(const string_t& strName, const int64_t& uValue);
		void SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val);
		string_t GetDescription();
		string_t GetValueDescription(const string_t& strName);
		void SetDescription(const string_t& strValue);
		void SetValueDescription(const string_t& strName, const string_t& strValue);
		ValueType_t GetValueType(const string_t& strName);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		Omega::IEnumString* EnumSubKeys();
		Omega::IEnumString* EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	class RootKey :
		public ObjectBase,
		public ::Registry::Manager,
		public IKey
	{
	public:
		void Init();

		BEGIN_INTERFACE_MAP(RootKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		OOBase::SmartPtr< ::Registry::Hive> m_system_hive;
		OOBase::SmartPtr< ::Registry::Hive> m_allusers_hive;
		OOBase::SmartPtr< ::Registry::Hive> m_localuser_hive;

		ObjectPtr<IKey> m_ptrSystemKey;
		ObjectPtr<IKey> m_ptrAllUsersKey;
		ObjectPtr<IKey> m_ptrLocalUserKey;

		string_t parse_subkey(const string_t& strSubKey, ObjectPtr<IKey>& ptrKey);
		int registry_access_check(const std::string& strdb, Omega::uint32_t channel_id, ::Registry::Hive::access_rights_t access_mask);

	// IRegistry members
	public:
		bool_t IsSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		string_t GetStringValue(const string_t& strName);
		int64_t GetIntegerValue(const string_t& strName);
		void GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer);
		void SetStringValue(const string_t& strName, const string_t& strValue);
		void SetIntegerValue(const string_t& strName, const int64_t& uValue);
		void SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val);
		string_t GetDescription();
		string_t GetValueDescription(const string_t& strName);
		void SetDescription(const string_t& strValue);
		void SetValueDescription(const string_t& strName, const string_t& strValue);
		ValueType_t GetValueType(const string_t& strName);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		Omega::IEnumString* EnumSubKeys();
		Omega::IEnumString* EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	static std::string get_db_dir(bool bSystem)
	{
#if defined(_WIN32)

		wchar_t szBuf[MAX_PATH] = {0};
		HRESULT hr;
		if (bSystem)
			hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
		else
			hr = SHGetFolderPathW(0,CSIDL_LOCAL_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);

		if FAILED(hr)
			OMEGA_THROW(string_t(("SHGetFolderPathW failed: " + OOBase::Win32::FormatMessage()).c_str(),false));

		if (!PathAppendW(szBuf,L"Omega Online"))
			OMEGA_THROW(string_t(("PathAppendW failed: " + OOBase::Win32::FormatMessage()).c_str(),false));

		if (!PathFileExistsW(szBuf))
		{
			if (!CreateDirectoryW(szBuf,NULL))
				OMEGA_THROW(string_t(("CreateDirectoryW failed: " + OOBase::Win32::FormatMessage()).c_str(),false));
		}

		std::string dir = OOBase::to_utf8(szBuf);
		if (*dir.rbegin() != '\\')
			dir += '\\';

#elif defined(HAVE_UNISTD_H)

		std::string dir;
		if (bSystem)
			dir = "/var/lib/omegaonline";
		else
		{
			OOSvrBase::pw_info pw(getuid());
			if (!pw)
				OMEGA_THROW(errno);

			dir = pw->pw_dir;
			dir += "/.omegaonline";
		}

		int flags;
		if (bSystem)
			flags = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		else
			flags = S_IRWXU | S_IRGRP;

		if (mkdir(dir.c_str(),flags) != 0)
		{
			if (errno != EEXIST)
				OMEGA_THROW(errno);
		}

		dir += "/";

#else
#error Fix me!
#endif

		return dir;
	}
}

void HiveKey::Init(::Registry::Hive* pHive, const Omega::string_t& strKey, const Omega::int64_t& key)
{
	m_pHive = pHive;
	m_strKey = strKey;
	m_key = key;
}

bool_t HiveKey::IsSubKey(const string_t& strSubKey)
{
	::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	int64_t uSubKey = 0;
	int err = m_pHive->open_key(m_key,uSubKey,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t HiveKey::IsValue(const string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::IsValue");

	byte_t v = 0;
	int err = m_pHive->get_value_type(m_key,OOBase::to_utf8(strName.c_str()),0,v);
	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsValue");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

int HiveKey::GetValueType_i(const string_t& strName, ValueType_t& vtype)
{
	byte_t value_type = 0;
	int err = m_pHive->get_value_type(m_key,OOBase::to_utf8(strName.c_str()),0,value_type);
	if (err != 0)
		return err;

	switch (value_type)
	{
	case 0:
		vtype = String;
		break;

	case 1:
		vtype = Integer;
		break;

	case 2:
		vtype = Binary;
		break;

	default:
		OMEGA_THROW(L"Registry value has invalid value type in the database");
	}

	return 0;
}

ValueType_t HiveKey::GetValueType(const string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueType");

	ValueType_t vtype;
	int err = GetValueType_i(strName,vtype);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueType");
	else if (err != 0)
		OMEGA_THROW(err);

	return vtype;
}

string_t HiveKey::GetStringValue(const string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetStringValue");

	std::string strValue;
	int err = m_pHive->get_string_value(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

int64_t HiveKey::GetIntegerValue(const string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetIntegerValue");

	int64_t uValue;
	int err = m_pHive->get_integer_value(m_key,OOBase::to_utf8(strName.c_str()),0,uValue);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);

	return uValue;
}

void HiveKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetBinaryValue");

	int err = m_pHive->get_binary_value(m_key,OOBase::to_utf8(strName.c_str()),0,cbLen,pBuffer);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetStringValue");

	int err = m_pHive->set_string_value(m_key,OOBase::to_utf8(strName.c_str()),0,OOBase::to_utf8(strValue.c_str()).c_str());
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetIntegerValue(const string_t& strName, const int64_t& value)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetIntegerValue");

	int err = m_pHive->set_integer_value(m_key,OOBase::to_utf8(strName.c_str()),0,value);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	int err = m_pHive->set_binary_value(m_key,OOBase::to_utf8(strName.c_str()),0,cbLen,val);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t HiveKey::GetDescription()
{
	std::string strValue;
	int err = m_pHive->get_description(m_key,0,strValue);
	if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

string_t HiveKey::GetValueDescription(const Omega::string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueDescription");

	std::string strValue;
	int err = m_pHive->get_value_description(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

void HiveKey::SetDescription(const Omega::string_t& strDesc)
{
	int err = m_pHive->set_description(m_key,0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	int err = m_pHive->set_value_description(m_key,OOBase::to_utf8(strValue.c_str()),0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strValue,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* HiveKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	int err;
	int64_t key;
	if (flags & IKey::Create)
		err = m_pHive->create_key(m_key,key,OOBase::to_utf8(strSubKey.c_str()),(flags & IKey::FailIfThere) ? true : false,::Registry::Hive::inherit_checks,0);
	else
		err = m_pHive->open_key(m_key,key,OOBase::to_utf8(strSubKey.c_str()),0);

	if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==EEXIST)
		::Registry::AlreadyExistsException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<HiveKey> > ptrNew = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrNew->Init(m_pHive,m_strKey + L"\\" + strSubKey,key);
	return ptrNew.AddRef();
}

Omega::IEnumString* HiveKey::EnumSubKeys()
{
	std::list<std::string> listSubKeys;
	int err = m_pHive->enum_subkeys(m_key,0,listSubKeys);
	if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err != 0)
		OMEGA_THROW(err);

	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();

	try
	{
		for (std::list<std::string>::const_iterator i=listSubKeys.begin();i!=listSubKeys.end();++i)
		{
			ptrEnum->Append(string_t(i->c_str(),true));
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

Omega::IEnumString* HiveKey::EnumValues()
{
	std::list<std::string> listValues;
	int err = m_pHive->enum_values(m_key,0,listValues);
	if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err != 0)
		OMEGA_THROW(err);

	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();

	try
	{
		for (std::list<std::string>::const_iterator i=listValues.begin();i!=listValues.end();++i)
		{
			ptrEnum->Append(string_t(i->c_str(),true));
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

void HiveKey::DeleteKey(const string_t& strSubKey)
{
	::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	int err = m_pHive->delete_key(m_key,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::DeleteValue(const string_t& strName)
{
	::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::DeleteValue");

	int err = m_pHive->delete_value(m_key,OOBase::to_utf8(strName.c_str()),0);
	if (err == ENOENT)
		::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void RootKey::Init()
{
	OMEGA_NEW(m_system_hive,::Registry::Hive(this,get_db_dir(true) + "system.regdb",::Registry::Hive::write_check | ::Registry::Hive::read_check));
	OMEGA_NEW(m_allusers_hive,::Registry::Hive(this,get_db_dir(true) + "all_users.regdb",::Registry::Hive::write_check));
	OMEGA_NEW(m_localuser_hive,::Registry::Hive(this,get_db_dir(false) + "user.regdb",0));

	if (!m_system_hive->open() || !m_allusers_hive->open() || !m_localuser_hive->open())
		OMEGA_THROW(L"Failed to open database files");

	// Add the default keys
	int err = ::Registry::Hive::init_system_defaults(m_system_hive.value());
	if (err != 0)
		OMEGA_THROW(err);

	err = ::Registry::Hive::init_allusers_defaults(m_allusers_hive.value());
	if (err != 0)
		OMEGA_THROW(err);

	ObjectPtr<ObjectImpl<HiveKey> > ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_system_hive.value(),L"",0);
	m_ptrSystemKey = static_cast<IKey*>(ptrKey);

	ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_allusers_hive.value(),L"\\All Users",0);
	m_ptrAllUsersKey = static_cast<IKey*>(ptrKey);

	ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_localuser_hive.value(),L"\\Local User",0);
	m_ptrLocalUserKey = static_cast<IKey*>(ptrKey);
}

int RootKey::registry_access_check(const std::string& /*strdb*/, Omega::uint32_t /*channel_id*/, ::Registry::Hive::access_rights_t /*access_mask*/)
{
	// Allow sqlite's underlying protection mechanism to sort it out
	return 0;
}

string_t RootKey::parse_subkey(const string_t& strSubKey, ObjectPtr<IKey>& ptrKey)
{
	// Parse strKey
	if (strSubKey == L"Local User" || strSubKey.Mid(0,11) == L"Local User\\")
	{
		ptrKey = m_ptrLocalUserKey;

		// Set the type and strip the start...
		if (strSubKey.Length() > 11)
			return strSubKey.Mid(11);
		else
			return string_t();
	}
	else if (strSubKey == L"All Users" || strSubKey.Mid(0,10) == L"All Users\\")
	{
		ptrKey = m_ptrAllUsersKey;

		// Set the type and strip the start...
		if (strSubKey.Length() > 10)
			return strSubKey.Mid(10);
		else
			return string_t();
	}
	else
	{
		ptrKey = m_ptrSystemKey;

		return strSubKey;
	}
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return true;

	if (!ptrKey)
		::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	return ptrKey->IsSubKey(strSubKey2);
}

bool_t RootKey::IsValue(const string_t& strName)
{
	return m_ptrSystemKey->IsValue(strName);
}

ValueType_t RootKey::GetValueType(const string_t& strName)
{
	return m_ptrSystemKey->GetValueType(strName);
}

string_t RootKey::GetStringValue(const string_t& strName)
{
	return m_ptrSystemKey->GetStringValue(strName);
}

int64_t RootKey::GetIntegerValue(const string_t& strName)
{
	return m_ptrSystemKey->GetIntegerValue(strName);
}

void RootKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	m_ptrSystemKey->GetBinaryValue(strName,cbLen,pBuffer);
}

void RootKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	m_ptrSystemKey->SetStringValue(strName,strValue);
}

void RootKey::SetIntegerValue(const string_t& strName, const int64_t& value)
{
	m_ptrSystemKey->SetIntegerValue(strName,value);
}

void RootKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	m_ptrSystemKey->SetBinaryValue(strName,cbLen,val);
}

string_t RootKey::GetDescription()
{
	return m_ptrSystemKey->GetDescription();
}

string_t RootKey::GetValueDescription(const Omega::string_t& strName)
{
	return m_ptrSystemKey->GetValueDescription(strName);
}

void RootKey::SetDescription(const Omega::string_t& strDesc)
{
	m_ptrSystemKey->SetDescription(strDesc);
}

void RootKey::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	m_ptrSystemKey->SetValueDescription(strValue,strDesc);
}

IKey* RootKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return ptrKey.AddRef();

	if (!ptrKey)
		::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	return ptrKey->OpenSubKey(strSubKey2,flags);
}

Omega::IEnumString* RootKey::EnumSubKeys()
{
	return m_ptrSystemKey->EnumSubKeys();
}

Omega::IEnumString* RootKey::EnumValues()
{
	return m_ptrSystemKey->EnumValues();
}

void RootKey::DeleteKey(const string_t& strSubKey)
{
	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		::Registry::AccessDeniedException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	if (!ptrKey)
		::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	return ptrKey->DeleteKey(strSubKey2);
}

void RootKey::DeleteValue(const string_t& strName)
{
	m_ptrSystemKey->DeleteValue(strName);
}

IKey* InterProcessService::GetRegistry()
{
	// Return a pointer to the singleton
	return SingletonObjectImpl<RootKey>::CreateInstancePtr().AddRef();
}

Activation::IRunningObjectTable* InterProcessService::GetRunningObjectTable()
{
	throw Omega::Remoting::IChannelClosedException::Create();
}

void InterProcessService::LaunchObjectApp(const guid_t&, const guid_t&, IObject*&)
{
	OMEGA_THROW(L"Invalid standalone function");
}

bool_t InterProcessService::HandleRequest(uint32_t)
{
	OMEGA_THROW(L"Invalid standalone function");
}

Remoting::IChannel* InterProcessService::OpenRemoteChannel(const string_t&)
{
	OMEGA_THROW(L"Invalid standalone function");
}

Remoting::IChannelSink* InterProcessService::OpenServerSink(const guid_t&, Remoting::IChannelSink*)
{
	throw Omega::Remoting::IChannelClosedException::Create();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::System::IInterProcessService*,OOSvrLite_GetIPS,0,())
{
	return SingletonObjectImpl<InterProcessService>::CreateInstancePtr().AddRef();
}
