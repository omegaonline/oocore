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
		std::set<Omega::string_t> EnumSubKeys();
		std::set<Omega::string_t> EnumValues();
		void DeleteKey(const string_t& strSubKey);
		void DeleteValue(const string_t& strName);
	};

	class MirrorKey : 
		public ObjectBase,
		public IKey
	{
	public:
		void Init(const string_t& strKey, IKey* pLocal, IKey* pSystem);
		
		BEGIN_INTERFACE_MAP(MirrorKey)
			INTERFACE_ENTRY(IKey)
		END_INTERFACE_MAP()

	private:
		string_t        m_strKey;
		ObjectPtr<IKey> m_ptrLocal;
		ObjectPtr<IKey> m_ptrSystem;
		
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
		std::set<string_t> EnumSubKeys();
		std::set<string_t> EnumValues();
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

	static std::string get_db_dir(bool bSystem)
	{
		// This all needs reworking in light of the config file changes
		void* TODO;

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

		if (!PathFileExistsW(szBuf) && !CreateDirectoryW(szBuf,NULL))
			OMEGA_THROW(string_t(("CreateDirectoryW failed: " + OOBase::Win32::FormatMessage()).c_str(),false));
		
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
			{
				int e = errno;
				OMEGA_THROW(e);
			}

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
			{
				int e = errno;
				OMEGA_THROW(e);
			}
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

std::set<Omega::string_t> HiveKey::EnumSubKeys()
{
	std::set<std::string> setSubKeys;
	int err = m_pHive->enum_subkeys(m_key,0,setSubKeys);
	if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutSubKeys;
	for (std::set<std::string>::const_iterator i=setSubKeys.begin();i!=setSubKeys.end();++i)
		setOutSubKeys.insert(string_t(i->c_str(),true));

	return setOutSubKeys;
}

std::set<Omega::string_t> HiveKey::EnumValues()
{
	std::set<std::string> setValues;
	int err = m_pHive->enum_values(m_key,0,setValues);
	if (err==EACCES)
		::Registry::AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err==ENOENT)
		::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> setOutValues;
	for (std::set<std::string>::const_iterator i=setValues.begin();i!=setValues.end();++i)
		setOutValues.insert(string_t(i->c_str(),true));

	return setOutValues;
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

void MirrorKey::Init(const string_t& strKey, IKey* pLocal, IKey* pSystem)
{
	m_strKey = strKey;
	m_ptrLocal = pLocal;
	m_ptrSystem = pSystem;
}

bool_t MirrorKey::IsSubKey(const string_t& strSubKey)
{
	return ((m_ptrLocal && m_ptrLocal->IsSubKey(strSubKey)) ||
			(m_ptrSystem && m_ptrSystem->IsSubKey(strSubKey)));
}

bool_t MirrorKey::IsValue(const string_t& strName)
{
	return ((m_ptrLocal && m_ptrLocal->IsValue(strName)) ||
			(m_ptrSystem && m_ptrSystem->IsValue(strName)));
}

string_t MirrorKey::GetStringValue(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetStringValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetStringValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	return string_t();
}

int64_t MirrorKey::GetIntegerValue(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetIntegerValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetIntegerValue(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	return 0;
}

void MirrorKey::GetBinaryValue(const string_t& strName, uint32_t& cbLen, byte_t* pBuffer)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetBinaryValue(strName,cbLen,pBuffer);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetBinaryValue(strName,cbLen,pBuffer);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
}

void MirrorKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	m_ptrLocal->SetStringValue(strName,strValue);
}

void MirrorKey::SetIntegerValue(const string_t& strName, const int64_t& uValue)
{
	m_ptrLocal->SetIntegerValue(strName,uValue);
}

void MirrorKey::SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val)
{
	m_ptrLocal->SetBinaryValue(strName,cbLen,val);
}

string_t MirrorKey::GetDescription()
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetDescription();
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetDescription();
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	return string_t();
}

string_t MirrorKey::GetValueDescription(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetValueDescription(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetValueDescription(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueDescription");
	return string_t();
}

void MirrorKey::SetDescription(const string_t& strValue)
{
	m_ptrLocal->SetDescription(strValue);
}

void MirrorKey::SetValueDescription(const string_t& strName, const string_t& strValue)
{
	m_ptrLocal->SetValueDescription(strName,strValue);
}

ValueType_t MirrorKey::GetValueType(const string_t& strName)
{
	if (m_ptrLocal)
	{
		try
		{
			return m_ptrLocal->GetValueType(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			return m_ptrSystem->GetValueType(strName);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	::Registry::NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	return 0;
}

IKey* MirrorKey::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	ObjectPtr<IKey> ptrNewLocal;
	ObjectPtr<IKey> ptrNewSystem;
	if (m_ptrLocal)
	{
		try
		{
			ptrNewLocal = m_ptrLocal->OpenSubKey(strSubKey,flags);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (m_ptrSystem)
	{
		try
		{
			ptrNewSystem = m_ptrSystem->OpenSubKey(strSubKey,IKey::OpenExisting);
		}
		catch (INotFoundException* pE)
		{
			pE->Release();
		}
	}

	if (!ptrNewLocal && !ptrNewSystem)
		::Registry::NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	
	ObjectPtr<ObjectImpl<MirrorKey> > ptrNew = ObjectImpl<MirrorKey>::CreateInstancePtr();
	ptrNew->Init(m_strKey + L"\\" + strSubKey,ptrNewLocal,ptrNewSystem);
	return ptrNew.AddRef();	
}

std::set<string_t> MirrorKey::EnumSubKeys()
{
	std::set<string_t> ret;
	if (m_ptrLocal)
	{
		ret = m_ptrLocal->EnumSubKeys();
	}

	if (m_ptrSystem)
	{
		std::set<string_t> ret2 = m_ptrSystem->EnumSubKeys();
		ret.insert(ret2.begin(),ret2.end());
	}

	return ret;
}

std::set<string_t> MirrorKey::EnumValues()
{
	std::set<string_t> ret;
	if (m_ptrLocal)
	{
		ret = m_ptrLocal->EnumValues();
	}

	if (m_ptrSystem)
	{
		std::set<string_t> ret2 = m_ptrSystem->EnumValues();
		ret.insert(ret2.begin(),ret2.end());
	}

	return ret;
}

void MirrorKey::DeleteKey(const string_t& strSubKey)
{
	m_ptrLocal->DeleteKey(strSubKey);
}

void MirrorKey::DeleteValue(const string_t& strName)
{
	m_ptrLocal->DeleteValue(strName);
}

void RootKey::Init()
{
	OMEGA_NEW(m_system_hive,::Registry::Hive(this,get_db_dir(true) + "system.regdb",::Registry::Hive::write_check | ::Registry::Hive::read_check));
	OMEGA_NEW(m_localuser_hive,::Registry::Hive(this,get_db_dir(false) + "user.regdb",0));

	if (!m_system_hive->open(SQLITE_OPEN_READWRITE) || !m_system_hive->open(SQLITE_OPEN_READONLY))
	{
		void* TODO; //  Generate a fake...
		OMEGA_THROW(L"Failed to open system registry database file");
	}

	if (!m_localuser_hive->open(SQLITE_OPEN_READWRITE))
	{
		void* TODO; //  Copy default_user...
		OMEGA_THROW(L"Failed to open database files");
	}

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
			strMirror = strSubKey.Mid(10);
		
		ObjectPtr<IKey> ptrMirror;
		try
		{
			// All Users
			ptrMirror = ObjectPtr<IKey>(L"\\All Users" + strMirror);
		}
		catch (Omega::Registry::INotFoundException* pE)
		{
			// We can ignore not found
			pE ->Release();
		}

		ObjectPtr<ObjectImpl<MirrorKey> > ptrNew = ObjectImpl<MirrorKey>::CreateInstancePtr();
		ptrNew->Init(L"\\" + strSubKey,m_ptrLocalUserKey,ptrMirror);
		ptrKey = ptrNew.AddRef();

		if (!strMirror.IsEmpty())
			strMirror = strMirror.Mid(1);

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

std::set<Omega::string_t> RootKey::EnumSubKeys()
{
	return m_ptrSystemKey->EnumSubKeys();
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
	return SingletonObjectImpl<RootKey>::CreateInstance();
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

OMEGA_DEFINE_EXPORTED_FUNCTION(OOCore::IInterProcessService*,OOSvrLite_GetIPS,0,())
{
	return SingletonObjectImpl<InterProcessService>::CreateInstance();
}
