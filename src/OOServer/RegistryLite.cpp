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

	// IKey members
	public:
		string_t GetName();
		bool_t IsSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		any_t GetValue(const string_t& strName);
		void SetValue(const string_t& strName, const any_t& value);
		string_t GetDescription();
		string_t GetValueDescription(const string_t& strName);
		void SetDescription(const string_t& strValue);
		void SetValueDescription(const string_t& strName, const string_t& strValue);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		Omega::Registry::IKey::string_set_t EnumSubKeys();
		Omega::Registry::IKey::string_set_t EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	template <typename T>
	class OmegaDestructor
	{
	public:
		static void destroy(T* ptr)
		{
			OMEGA_DELETE(T,ptr);
		}
	};

	class RootKey :
			public ObjectBase,
			public ::Registry::Manager,
			public IKey
	{
	protected:
		void Init_Once();

		BEGIN_INTERFACE_MAP(RootKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		OOBase::SmartPtr< ::Registry::Hive,OmegaDestructor< ::Registry::Hive> > m_system_hive;
		OOBase::SmartPtr< ::Registry::Hive,OmegaDestructor< ::Registry::Hive> > m_localuser_hive;

		ObjectPtr<IKey> m_ptrSystemKey;
		ObjectPtr<IKey> m_ptrLocalUserKey;

		string_t parse_subkey(const string_t& strSubKey, ObjectPtr<IKey>& ptrKey);
		int registry_access_check(const OOBase::string& strdb, Omega::uint32_t channel_id, ::Registry::Hive::access_rights_t access_mask);

	// IKey members
	public:
		string_t GetName();
		bool_t IsSubKey(const string_t& strSubKey);
		bool_t IsValue(const string_t& strName);
		any_t GetValue(const string_t& strName);
		void SetValue(const string_t& strName, const any_t& value);
		string_t GetDescription();
		string_t GetValueDescription(const string_t& strName);
		void SetDescription(const string_t& strValue);
		void SetValueDescription(const string_t& strName, const string_t& strValue);
		IKey* OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags = OpenExisting);
		Omega::Registry::IKey::string_set_t EnumSubKeys();
		Omega::Registry::IKey::string_set_t EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	static OOBase::string get_db_dir(InterProcessService* pIPS)
	{
		OOBase::string dir;
		pIPS->GetArg(L"regdb_path").ToUTF8(dir);

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

string_t HiveKey::GetName()
{
	return m_strKey + L"/";
}

bool_t HiveKey::IsSubKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	int64_t uSubKey = 0;
	int err = m_pHive->open_key(m_key,uSubKey,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t HiveKey::IsValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->value_exists(m_key,OOBase::to_utf8(strName.c_str()),0);
	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

any_t HiveKey::GetValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	OOBase::string strValue;
	int err = m_pHive->get_value(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

void HiveKey::SetValue(const string_t& strName, const any_t& value)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->set_value(m_key,OOBase::to_utf8(strName.c_str()),0,OOBase::to_utf8(value.cast<string_t>().c_str()).c_str());
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t HiveKey::GetDescription()
{
	OOBase::string strValue;
	int err = m_pHive->get_description(m_key,0,strValue);
	if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

string_t HiveKey::GetValueDescription(const Omega::string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	OOBase::string strValue;
	int err = m_pHive->get_value_description(m_key,OOBase::to_utf8(strName.c_str()),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	return string_t(strValue.c_str(),true);
}

void HiveKey::SetDescription(const Omega::string_t& strDesc)
{
	int err = m_pHive->set_description(m_key,0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	int err = m_pHive->set_value_description(m_key,OOBase::to_utf8(strValue.c_str()),0,OOBase::to_utf8(strDesc.c_str()));
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strValue);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* HiveKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	int64_t key;
	int err = m_pHive->create_key(m_key,key,OOBase::to_utf8(strSubKey.c_str()),flags,::Registry::Hive::inherit_checks,0);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EEXIST)
		User::Registry::AlreadyExistsException::Throw(GetName() + strSubKey);
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName() + strSubKey);
	else if (err != 0)
		OMEGA_THROW(err);

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<HiveKey> > ptrNew = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrNew->Init(m_pHive,GetName() + strSubKey,key);
	return ptrNew.AddRef();
}

Omega::Registry::IKey::string_set_t HiveKey::EnumSubKeys()
{
	::Registry::Hive::setType setSubKeys;
	int err = m_pHive->enum_subkeys(m_key,0,setSubKeys);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	Omega::Registry::IKey::string_set_t setOutSubKeys;
	for (::Registry::Hive::setType::const_iterator i=setSubKeys.begin(); i!=setSubKeys.end(); ++i)
		setOutSubKeys.insert(string_t(i->c_str(),true));

	return setOutSubKeys;
}

Omega::Registry::IKey::string_set_t HiveKey::EnumValues()
{
	::Registry::Hive::setType setValues;
	int err = m_pHive->enum_values(m_key,0,setValues);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);

	Omega::Registry::IKey::string_set_t setOutValues;
	for (::Registry::Hive::setType::const_iterator i=setValues.begin(); i!=setValues.end(); ++i)
		setOutValues.insert(string_t(i->c_str(),true));

	return setOutValues;
}

void HiveKey::DeleteKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	int err = m_pHive->delete_key(m_key,OOBase::to_utf8(strSubKey.c_str()),0);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(GetName() + strSubKey);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName() + strSubKey);
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::DeleteValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->delete_value(m_key,OOBase::to_utf8(strName.c_str()),0);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err != 0)
		OMEGA_THROW(err);
}

void RootKey::Init_Once()
{
	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstancePtr();

	OMEGA_NEW_T(::Registry::Hive,m_system_hive,::Registry::Hive(this,get_db_dir(ptrIPS) + "system.regdb"));

	OOBase::string s;
	ptrIPS->GetArg(L"user_regdb").ToNative(s);

	OMEGA_NEW_T(::Registry::Hive,m_localuser_hive,::Registry::Hive(this,s));

	if (!m_system_hive->open(SQLITE_OPEN_READWRITE) || !m_system_hive->open(SQLITE_OPEN_READONLY))
		OMEGA_THROW("Failed to open system registry database file");

	if (!m_localuser_hive->open(SQLITE_OPEN_READWRITE))
		OMEGA_THROW("Failed to open database files");

	ObjectPtr<ObjectImpl<HiveKey> > ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_system_hive,string_t(),0);
	m_ptrSystemKey = static_cast<IKey*>(ptrKey);

	ptrKey = ObjectImpl<HiveKey>::CreateInstancePtr();
	ptrKey->Init(m_localuser_hive,L"/Local User",0);
	m_ptrLocalUserKey = static_cast<IKey*>(ptrKey);
}

int RootKey::registry_access_check(const OOBase::string& /*strdb*/, Omega::uint32_t /*channel_id*/, ::Registry::Hive::access_rights_t /*access_mask*/)
{
	// Allow sqlite's underlying protection mechanism to sort it out
	return 0;
}

string_t RootKey::parse_subkey(const string_t& strSubKey, ObjectPtr<IKey>& ptrKey)
{
	// Parse strKey
	if (strSubKey == L"Local User" || strSubKey.Mid(0,11) == L"Local User/")
	{
		string_t strMirror;

		// Set the type and strip the start...
		if (strSubKey.Length() > 10)
			strMirror = strSubKey.Mid(11);

		ObjectPtr<IKey> ptrMirror = ObjectPtr<IKey>(L"/All Users");

		ObjectPtr<ObjectImpl<User::Registry::MirrorKey> > ptrNew = ObjectImpl<User::Registry::MirrorKey>::CreateInstancePtr();
		ptrNew->Init(L"/Local User",m_ptrLocalUserKey,ptrMirror);
		ptrKey.Attach(ptrNew.AddRef());

		return strMirror;
	}
	else
	{
		ptrKey = m_ptrSystemKey;

		return strSubKey;
	}
}

string_t RootKey::GetName()
{
	return string_t(L"/");
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return true;

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(GetName() + strSubKey);

	return ptrKey->IsSubKey(strSubKey2);
}

bool_t RootKey::IsValue(const string_t& strName)
{
	return m_ptrSystemKey->IsValue(strName);
}

any_t RootKey::GetValue(const string_t& strName)
{
	return m_ptrSystemKey->GetValue(strName);
}

void RootKey::SetValue(const string_t& strName, const any_t& value)
{
	m_ptrSystemKey->SetValue(strName,value);
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
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return ptrKey.AddRef();

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(GetName() + strSubKey);

	return ptrKey->OpenSubKey(strSubKey2,flags);
}

Omega::Registry::IKey::string_set_t RootKey::EnumSubKeys()
{
	Omega::Registry::IKey::string_set_t ret = m_ptrSystemKey->EnumSubKeys();

	// Add the local user key, although it doesn't really exist...
	ret.insert(L"Local User");

	return ret;
}

Omega::Registry::IKey::string_set_t RootKey::EnumValues()
{
	return m_ptrSystemKey->EnumValues();
}

void RootKey::DeleteKey(const string_t& strSubKey)
{
	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		User::Registry::AccessDeniedException::Throw(GetName() + strSubKey);

	if (!ptrKey)
		User::Registry::NotFoundException::Throw(GetName() + strSubKey);

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
