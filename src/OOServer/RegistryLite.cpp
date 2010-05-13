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
#include "IPSLite.h"
#include "RegistryHive.h"

#if defined(_WIN32)
#include <shlwapi.h>
#include <shlobj.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <sys/stat.h>
#endif

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
		std::set<Omega::string_t> EnumSubKeys();
		std::set<Omega::string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	class RootKey :
			public ObjectBase,
			public ::Registry::Manager,
			public IKey
	{
	protected:
		void InitOnce();

		BEGIN_INTERFACE_MAP(RootKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		OOBase::SmartPtr< ::Registry::Hive> m_system_hive;
		OOBase::SmartPtr< ::Registry::Hive> m_localuser_hive;

		ObjectPtr<IKey> m_ptrSystemKey;
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
		std::set<Omega::string_t> EnumSubKeys();
		std::set<Omega::string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	static std::string get_db_dir(InterProcessService* pIPS)
	{
		std::string dir = pIPS->GetArg(L"regdb_path").ToUTF8();
		if (!dir.empty())
		{
#if defined(_WIN32)
			if (*dir.rbegin() != '\\')
				dir += '\\';
#else
			if (*dir.rbegin() != '/')
				dir += '/';
#endif
		}
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
	User::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	int64_t uSubKey = 0;
	int err = m_pHive->open_key(m_key,uSubKey,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t HiveKey::IsValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::IsValue");

	byte_t v = 0;
	int err = m_pHive->get_value_type(m_key,OOBase::to_utf8(strName.c_str()),0,v);
	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsValue");
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
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueType");

	ValueType_t vtype;
	int err = GetValueType_i(strName,vtype);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueType");
	else if (err != 0)
		OMEGA_THROW(err);

	return vtype;
}

string_t HiveKey::GetStringValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetStringValue");

	std::string strValue;
	int err = m_pHive->get_string_value(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			User::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

int64_t HiveKey::GetIntegerValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetIntegerValue");

	int64_t uValue;
	int err = m_pHive->get_integer_value(m_key,OOBase::to_utf8(strName.c_str()),0,uValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			User::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);

	return uValue;
}

void HiveKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetBinaryValue");

	int err = m_pHive->get_binary_value(m_key,OOBase::to_utf8(strName.c_str()),0,cbLen,pBuffer);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			User::Registry::WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetStringValue");

	int err = m_pHive->set_string_value(m_key,OOBase::to_utf8(strName.c_str()),0,OOBase::to_utf8(strValue.c_str()).c_str());
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetIntegerValue(const string_t& strName, const int64_t& value)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetIntegerValue");

	int err = m_pHive->set_integer_value(m_key,OOBase::to_utf8(strName.c_str()),0,value);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	int err = m_pHive->set_binary_value(m_key,OOBase::to_utf8(strName.c_str()),0,cbLen,val);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t HiveKey::GetDescription()
{
	std::string strValue;
	int err = m_pHive->get_description(m_key,0,strValue);
	if (err==ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

string_t HiveKey::GetValueDescription(const Omega::string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueDescription");

	std::string strValue;
	int err = m_pHive->get_value_description(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

void HiveKey::SetDescription(const Omega::string_t& strDesc)
{
	int err = m_pHive->set_description(m_key,0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	int err = m_pHive->set_value_description(m_key,OOBase::to_utf8(strValue.c_str()),0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strValue,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* HiveKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	int64_t key;
	int err = m_pHive->create_key(m_key,key,OOBase::to_utf8(strSubKey.c_str()),flags,::Registry::Hive::inherit_checks,0);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==EEXIST)
		User::Registry::AlreadyExistsException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<HiveKey> > ptrNew = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrNew->Init(m_pHive,m_strKey + L"\\" + strSubKey,key);
	return ptrNew.AddRef();
}

std::set<Omega::string_t> HiveKey::EnumSubKeys()
{
	std::set<std::string> setSubKeys;
	int err = m_pHive->enum_subkeys(m_key,0,setSubKeys);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutSubKeys;
	for (std::set<std::string>::const_iterator i=setSubKeys.begin(); i!=setSubKeys.end(); ++i)
		setOutSubKeys.insert(string_t(i->c_str(),true));

	return setOutSubKeys;
}

std::set<Omega::string_t> HiveKey::EnumValues()
{
	std::set<std::string> setValues;
	int err = m_pHive->enum_values(m_key,0,setValues);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutValues;
	for (std::set<std::string>::const_iterator i=setValues.begin(); i!=setValues.end(); ++i)
		setOutValues.insert(string_t(i->c_str(),true));

	return setOutValues;
}

void HiveKey::DeleteKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	int err = m_pHive->delete_key(m_key,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::DeleteValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::DeleteValue");

	int err = m_pHive->delete_value(m_key,OOBase::to_utf8(strName.c_str()),0);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void RootKey::InitOnce()
{
	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstancePtr();

	OMEGA_NEW(m_system_hive,::Registry::Hive(this,get_db_dir(ptrIPS) + "system.regdb",::Registry::Hive::write_check | ::Registry::Hive::read_check));
	OMEGA_NEW(m_localuser_hive,::Registry::Hive(this,ptrIPS->GetArg(L"user_regdb").ToUTF8(),0));

	if (!m_system_hive->open(SQLITE_OPEN_READWRITE) || !m_system_hive->open(SQLITE_OPEN_READONLY))
		OMEGA_THROW(L"Failed to open system registry database file");
	
	if (!m_localuser_hive->open(SQLITE_OPEN_READWRITE))
		OMEGA_THROW(L"Failed to open database files");
	
	ObjectPtr<ObjectImpl<HiveKey> > ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_system_hive,string_t(),0);
	m_ptrSystemKey = static_cast<IKey*>(ptrKey);

	ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_localuser_hive,L"\\Local User",0);
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
		string_t strMirror;

		// Set the type and strip the start...
		if (strSubKey.Length() > 10)
			strMirror = strSubKey.Mid(11);

		ObjectPtr<IKey> ptrMirror = ObjectPtr<IKey>(L"\\All Users");
		
		ObjectPtr<ObjectImpl<User::Registry::MirrorKey> > ptrNew = ObjectImpl<User::Registry::MirrorKey>::CreateInstancePtr();
		ptrNew->Init(L"\\Local User",m_ptrLocalUserKey,ptrMirror);
		ptrKey.Attach(ptrNew.AddRef());

		return strMirror;
	}
	else
	{
		ptrKey = m_ptrSystemKey;

		return strSubKey;
	}
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return true;

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

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
	User::Registry::BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return ptrKey.AddRef();

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	return ptrKey->OpenSubKey(strSubKey2,flags);
}

std::set<Omega::string_t> RootKey::EnumSubKeys()
{
	std::set<Omega::string_t> ret = m_ptrSystemKey->EnumSubKeys();
	
	// Add the local user key, although it doesn't really exist...
	ret.insert(L"Local User");

	return ret;
}

std::set<Omega::string_t> RootKey::EnumValues()
{
	return m_ptrSystemKey->EnumValues();
}

void RootKey::DeleteKey(const string_t& strSubKey)
{
	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		User::Registry::AccessDeniedException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	return ptrKey->DeleteKey(strSubKey2);
}

void RootKey::DeleteValue(const string_t& strName)
{
	m_ptrSystemKey->DeleteValue(strName);
}

IKey* InterProcessService::GetRegistry()
{
	// Return a pointer to the singleton
	ObjectPtr<SingletonObjectImpl<RootKey> > ptrKey = SingletonObjectImpl<RootKey>::CreateInstancePtr();
	return ptrKey.AddRef();
}

#include "MirrorKey.inl"
