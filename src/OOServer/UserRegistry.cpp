///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

#include "OOServer_User.h"

#include "UserRegistry.h"
#include "UserManager.h"
#include "SpawnedProcess.h"

using namespace Omega;
using namespace Omega::Registry;
using namespace OTL;

namespace User
{
	#include "RegistryCmn.h"

	namespace Registry
	{
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
	}
}

using namespace User;
using namespace User::Registry;

void Key::Init(Manager* pManager, const Omega::string_t& strKey, const Omega::int64_t& key, Omega::byte_t type)
{
	m_pManager = pManager;
	m_strKey = strKey;
	m_key = key;
	m_type = type;
}

bool_t Key::IsSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::KeyExists));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");

	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		return false;
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	return true;
}

bool_t Key::IsValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::IsValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::ValueType));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err==ENOENT)
		return false;
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsValue");
	else if (err != 0)
		OMEGA_THROW(err);

	Omega::byte_t value_type;
	if (!response->read(value_type))
		OMEGA_THROW(response->last_error());

	return true;
}

int Key::GetValueType_i(const string_t& strName, ValueType_t& vtype)
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::ValueType));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err != 0)
		return err;

	Omega::byte_t value_type = 0;
	if (!response->read(value_type))
		OMEGA_THROW(response->last_error());

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

ValueType_t Key::GetValueType(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueType");

	ValueType_t vtype;
	int err = GetValueType_i(strName,vtype);
	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueType");
	else if (err != 0)
		OMEGA_THROW(err);

	return vtype;
}

string_t Key::GetStringValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetStringValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::GetStringValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);

	std::string strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

int64_t Key::GetIntegerValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetIntegerValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::GetIntegerValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetIntegerValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);

	Omega::int64_t uValue = 0;
	if (!response->read(uValue))
		OMEGA_THROW(response->last_error());

	return uValue;
}

void Key::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetBinaryValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::GetBinaryValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	request.write(cbLen);
	bool bNoDataBack = (cbLen == 0);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err == EINVAL)
	{
		ValueType_t vtype;
		err = GetValueType_i(strName,vtype);
		if (err == 0)
			WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetBinaryValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);

	if (!response->read(cbLen))
		OMEGA_THROW(response->last_error());

	if (!bNoDataBack)
	{
		if (!response->read_bytes(pBuffer,cbLen))
			OMEGA_THROW(response->last_error());
	}
}

void Key::SetStringValue(const string_t& strName, const string_t& strValue)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetStringValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::SetStringValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	request.write(strValue.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetIntegerValue(const string_t& strName, const int64_t& value)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetIntegerValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::SetIntegerValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	request.write(value);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::SetBinaryValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	request.write_bytes(val,cbLen);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

string_t Key::GetDescription()
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::GetDescription));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err==ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	std::string strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

string_t Key::GetValueDescription(const Omega::string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueDescription");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::GetValueDescription));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);

	std::string strValue;
	if (!response->read(strValue))
		OMEGA_THROW(response->last_error());

	return string_t(strValue.c_str(),true);
}

void Key::SetDescription(const Omega::string_t& strDesc)
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::SetDescription));
	request.write(m_key);
	request.write(m_type);
	request.write(strDesc.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetValueDescription(const Omega::string_t& strValue, const Omega::string_t& strDesc)
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::SetValueDescription));
	request.write(m_key);
	request.write(m_type);
	request.write(strValue.ToUTF8().c_str());
	request.write(strDesc.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strValue,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetValueDescription");
	else if (err != 0)
		OMEGA_THROW(err);
}

IKey* Key::OpenSubKey(const string_t& strSubKey, IKey::OpenFlags_t flags)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::CreateKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.ToUTF8().c_str());
	request.write((flags & IKey::Create) ? true : false);
	request.write((flags & IKey::FailIfThere) ? true : false);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());
	
	if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==EEXIST)
		AlreadyExistsException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	Omega::int64_t key = 0;
	Omega::byte_t type = 255;
	if (!response->read(key) || !response->read(type))
		OMEGA_THROW(response->last_error());

	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<Key> > ptrNew = ObjectImpl<Key>::CreateInstancePtr();
	ptrNew->Init(m_pManager,m_strKey + L"\\" + strSubKey,key,type);

	// See if we need a mirror key
	if (m_key == 0 && m_type == 0 && type != 0)
	{
		// Local user, strip the start...
		assert(strSubKey == L"Local User" || strSubKey.Left(11) == L"Local User\\");

		string_t strMirror;
		if (strSubKey.Length() > 10)
			strMirror = strSubKey.Mid(10);
		
		ObjectPtr<IKey> ptrMirror;
		try
		{
			if (type == 1)
			{
				// Sandbox
				ptrMirror = ObjectPtr<IKey>(L"\\Sandbox" + strMirror);
			}
			else if (type == 2)
			{
				// All Users
				ptrMirror = ObjectPtr<IKey>(L"\\All Users" + strMirror);
			}
		}
		catch (Omega::Registry::INotFoundException* pE)
		{
			// We can ignore not found
			pE ->Release();
		}

		ObjectPtr<ObjectImpl<MirrorKey> > ptrNew2 = ObjectImpl<MirrorKey>::CreateInstancePtr();
		ptrNew2->Init(m_strKey + L"\\" + strSubKey,ptrNew,ptrMirror);
		return ptrNew2.AddRef();
	}
	else
	{
		return ptrNew.AddRef();		
	}
}

std::set<Omega::string_t> Key::EnumSubKeys()
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::EnumSubKeys));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());
	
	if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> sub_keys;
		
	try
	{
		for (;;)
		{
			std::string strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			sub_keys.insert(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return sub_keys;
}

std::set<Omega::string_t> Key::EnumValues()
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::EnumValues));
	request.write(m_key);
	request.write(m_type);
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err != 0)
		OMEGA_THROW(err);

	std::set<Omega::string_t> values;
		
	try
	{
		for (;;)
		{
			std::string strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			values.insert(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return values;
}

void Key::DeleteKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::DeleteKey));
	request.write(m_key);
	request.write(m_type);
	request.write(strSubKey.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::DeleteValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::DeleteValue");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::DeleteValue));
	request.write(m_key);
	request.write(m_type);
	request.write(strName.ToUTF8().c_str());
	if (request.last_error() != 0)
		OMEGA_THROW(request.last_error());

	OOBase::SmartPtr<OOBase::CDRStream> response(m_pManager->sendrecv_root(request,TypeInfo::Synchronous));
	if (!response)
		OMEGA_THROW(L"No response from root");
	
	int err = 0;
	if (!response->read(err))
		OMEGA_THROW(response->last_error());

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::DeleteValue");
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

	NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
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

	NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
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

	NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
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

	NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetDescription");
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

	NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueDescription");
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

	NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
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
		NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	
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
