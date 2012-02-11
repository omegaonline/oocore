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
			public IProvideObjectInfoImpl<HiveKey>,
			public IKey
	{
	public:
		void Init(Db::Hive* pHive, const Omega::string_t& strKey, const Omega::int64_t& key);

		BEGIN_INTERFACE_MAP(HiveKey)
			INTERFACE_ENTRY(IKey)
			INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
		END_INTERFACE_MAP()

	private:
		Db::Hive* m_pHive;
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
		std::set<Omega::string_t> EnumSubKeys();
		std::set<Omega::string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	class RootKey :
			public ObjectBase,
			public Db::Manager,
			public IProvideObjectInfoImpl<HiveKey>,
			public IKey
	{
	protected:
		void Init_Once();

		BEGIN_INTERFACE_MAP(RootKey)
			INTERFACE_ENTRY(IKey)
			INTERFACE_ENTRY(Omega::TypeInfo::IProvideObjectInfo)
		END_INTERFACE_MAP()

	private:
		OOBase::SmartPtr< Db::Hive> m_system_hive;
		OOBase::SmartPtr< Db::Hive> m_localuser_hive;

		ObjectPtr<IKey> m_ptrSystemKey;
		ObjectPtr<IKey> m_ptrLocalUserKey;

		string_t parse_subkey(const string_t& strSubKey, IKey*& pKey);
		int registry_access_check(const char* pszdb, Omega::uint32_t channel_id, Db::Hive::access_rights_t access_mask);

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
		std::set<Omega::string_t> EnumSubKeys();
		std::set<Omega::string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	static void get_db_dir(InterProcessService* pIPS, OOBase::LocalString& dir)
	{
		int err = dir.assign(pIPS->GetArg("regdb_path").c_str());
		if (err != 0)
			OMEGA_THROW(err);
		
		if (!dir.empty())
		{
			if ((err = OOBase::Paths::CorrectDirSeparators(dir)) == 0)
				err = OOBase::Paths::AppendDirSeparator(dir);
		}

		if (err != 0)
			OMEGA_THROW(err);
	}
}

void HiveKey::Init(Db::Hive* pHive, const Omega::string_t& strKey, const Omega::int64_t& key)
{
	m_pHive = pHive;
	m_strKey = strKey;
	m_key = key;
}

string_t HiveKey::GetName()
{
	return m_strKey;
}

bool_t HiveKey::IsSubKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	int64_t uSubKey = 0;
	int err = m_pHive->open_key(m_key,uSubKey,strSubKey.c_str(),0);
	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t HiveKey::IsValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->value_exists(m_key,strName.c_str(),0);
	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

any_t HiveKey::GetValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	OOBase::LocalString strValue;
	int err = m_pHive->get_value(m_key,strName.c_str(),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return strValue.c_str();
}

void HiveKey::SetValue(const string_t& strName, const any_t& value)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->set_value(m_key,strName.c_str(),0,value.cast<string_t>().c_str());
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t HiveKey::GetDescription()
{
	OOBase::LocalString strValue;
	int err = m_pHive->get_description(m_key,0,strValue);
	if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return strValue.c_str();
}

string_t HiveKey::GetValueDescription(const Omega::string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	OOBase::LocalString strValue;
	int err = m_pHive->get_value_description(m_key,strName.c_str(),0,strValue);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	return strValue.c_str();
}

void HiveKey::SetDescription(const Omega::string_t& strDesc)
{
	int err = m_pHive->set_description(m_key,0,strDesc.c_str());
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

void HiveKey::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	int err = m_pHive->set_value_description(m_key,strValue.c_str(),0,strDesc.c_str());
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strValue);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* HiveKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);
	
	string_t strFullKey = GetName();
	if (!strFullKey.IsEmpty())
		strFullKey += '/';
	strFullKey += strSubKey;

	int64_t key;
	int err = m_pHive->create_key(m_key,key,strSubKey.c_str(),flags,Db::Hive::inherit_checks,0);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(strFullKey);
	else if (err==EEXIST)
		User::Registry::AlreadyExistsException::Throw(strFullKey);
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(strFullKey);
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<HiveKey> > ptrNew = ObjectImpl<HiveKey>::CreateInstance();
	ptrNew->Init(m_pHive,strFullKey,key);
	return ptrNew.AddRef();
}

std::set<Omega::string_t> HiveKey::EnumSubKeys()
{
	Db::Hive::registry_set_t setSubKeys;
	int err = m_pHive->enum_subkeys(m_key,0,setSubKeys);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutSubKeys;

	for (OOBase::String i;setSubKeys.pop(&i);)
		setOutSubKeys.insert(i.c_str());

	return setOutSubKeys;
}

std::set<Omega::string_t> HiveKey::EnumValues()
{
	Db::Hive::registry_set_t setValues;
	int err = m_pHive->enum_values(m_key,0,setValues);
	if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==ENOENT)
		User::Registry::NotFoundException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutValues;

	for (OOBase::String i;setValues.pop(&i);)
		setOutValues.insert(i.c_str());

	return setOutValues;
}

void HiveKey::DeleteKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);
	
	int err = m_pHive->delete_key(m_key,strSubKey.c_str(),0);
	if (err != 0)
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += '/';
		strFullKey += strSubKey;
		
		if (err == ENOENT)
			User::Registry::NotFoundException::Throw(strFullKey);
		else if (err==EACCES)
			User::Registry::AccessDeniedException::Throw(strFullKey);
		else if (err==EIO)
			OMEGA_THROW("Unexpected registry error");
		else
			OMEGA_THROW(err);
	}
}

void HiveKey::DeleteValue(const string_t& strName)
{
	User::Registry::BadNameException::ValidateValue(strName);

	int err = m_pHive->delete_value(m_key,strName.c_str(),0);
	if (err == ENOENT)
		User::Registry::NotFoundException::Throw(strName);
	else if (err==EACCES)
		User::Registry::AccessDeniedException::Throw(GetName());
	else if (err==EIO)
		OMEGA_THROW("Unexpected registry error");
	else if (err != 0)
		OMEGA_THROW(err);
}

void RootKey::Init_Once()
{
	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstance();

	OOBase::LocalString dir;
	get_db_dir(ptrIPS,dir);

	int err = dir.append("system.regdb");
	if (err != 0)
		OMEGA_THROW(err);

	m_system_hive = new (std::nothrow) Db::Hive(this,dir.c_str());
	if (!m_system_hive)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	if (!m_system_hive->open(SQLITE_OPEN_READWRITE) && !m_system_hive->open(SQLITE_OPEN_READONLY))
		OMEGA_THROW("Failed to open system registry database file");

	m_localuser_hive = new (std::nothrow) Db::Hive(this,ptrIPS->GetArg("user_regdb").c_str());
	if (!m_localuser_hive)
		OMEGA_THROW(ERROR_OUTOFMEMORY);

	if (!m_localuser_hive->open(SQLITE_OPEN_READWRITE))
		OMEGA_THROW("Failed to open database files");

	ObjectPtr<ObjectImpl<HiveKey> > ptrKey = ObjectImpl<HiveKey>::CreateInstance();
	ptrKey->Init(m_system_hive,string_t(),0);
	m_ptrSystemKey = ptrKey.AddRef();

	ptrKey = ObjectImpl<HiveKey>::CreateInstance();
	ptrKey->Init(m_localuser_hive,string_t::constant("Local User"),0);
	m_ptrLocalUserKey = ptrKey.AddRef();
}

int RootKey::registry_access_check(const char* /*strdb*/, Omega::uint32_t /*channel_id*/, Db::Hive::access_rights_t /*access_mask*/)
{
	// Allow sqlite's underlying protection mechanism to sort it out
	return 0;
}

string_t RootKey::parse_subkey(const string_t& strSubKey, IKey*& pKey)
{
	// Parse strKey
	if (strSubKey == "Local User" || strSubKey.Mid(0,11) == "Local User/")
	{
		string_t strMirror;

		// Set the type and strip the start...
		if (strSubKey.Length() > 10)
			strMirror = strSubKey.Mid(11);

		ObjectPtr<IKey> ptrMirror = ObjectPtr<IKey>(string_t::constant("All Users"));

		ObjectPtr<ObjectImpl<User::Registry::MirrorKey> > ptrNew = ObjectImpl<User::Registry::MirrorKey>::CreateInstance();
		ptrNew->Init(string_t::constant("Local User"),m_ptrLocalUserKey,ptrMirror);
		pKey = ptrNew.AddRef();

		return strMirror;
	}
	else
	{
		pKey = m_ptrSystemKey.AddRef();

		return strSubKey;
	}
}

string_t RootKey::GetName()
{
	return string_t();
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	User::Registry::BadNameException::ValidateSubKey(strSubKey);

	ObjectPtr<IKey> ptrKey;
	string_t strSubKey2 = parse_subkey(strSubKey,ptrKey);
	if (strSubKey2.IsEmpty())
		return true;

	if (!ptrKey)
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += '/';
		strFullKey += strSubKey;
		
		User::Registry::NotFoundException::Throw(strFullKey);
	}

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
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += '/';
		strFullKey += strSubKey;
		
		User::Registry::NotFoundException::Throw(strFullKey);
	}

	return ptrKey->OpenSubKey(strSubKey2,flags);
}

std::set<Omega::string_t> RootKey::EnumSubKeys()
{
	std::set<Omega::string_t> ret = m_ptrSystemKey->EnumSubKeys();

	// Add the local user key, although it doesn't really exist...
	ret.insert(string_t::constant("Local User"));

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
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += '/';
		strFullKey += strSubKey;
		
		User::Registry::AccessDeniedException::Throw(strFullKey);
	}

	if (!ptrKey)
	{
		string_t strFullKey = GetName();
		if (!strFullKey.IsEmpty())
			strFullKey += '/';
		strFullKey += strSubKey;
		
		User::Registry::NotFoundException::Throw(strFullKey);
	}

	return ptrKey->DeleteKey(strSubKey2);
}

void RootKey::DeleteValue(const string_t& strName)
{
	m_ptrSystemKey->DeleteValue(strName);
}

IKey* InterProcessService::GetRegistry()
{
	// Return a pointer to the singleton
	ObjectPtr<SingletonObjectImpl<RootKey> > ptrKey = SingletonObjectImpl<RootKey>::CreateInstance();
	return ptrKey.AddRef();
}

#include "MirrorKey.inl"
