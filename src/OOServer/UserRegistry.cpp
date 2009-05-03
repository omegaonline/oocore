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

		static void ValidateSubKey(const string_t& strSubKey, const string_t& strSource)
		{
			if (strSubKey.IsEmpty() ||
				strSubKey == L"\\" ||
				strSubKey.Right(1) == L"\\" ||
				strSubKey.Find(L"\\\\") != string_t::npos)
			{
				Throw(strSubKey,strSource);
			}
		}

		static void ValidateValue(const string_t& strName, const string_t& strSource)
		{
			if (strName.IsEmpty() || 
				strName.Find(L'\\') != string_t::npos)
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
			throw static_cast<IBadNameException*>(pRE);
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
				tp = L"String";
			else if (actual_type==Integer)
				tp = L"Integer";
			else if (actual_type==Binary)
				tp = L"Binary";

			pRE->m_strDesc = string_t::Format(L"Incorrect registry value type, actual value type is %ls.",tp.c_str());

			throw static_cast<IWrongValueTypeException*>(pRE);
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
			throw static_cast<INotFoundException*>(pRE);
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
			throw static_cast<IAlreadyExistsException*>(pRE);
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
			throw static_cast<IAccessDeniedException*>(pRE);
		}
	};
}
}

using namespace User;
using namespace User::Registry;

void Key::Init(Manager* pManager, const Omega::string_t& strKey, const Omega::int64_t& key)
{
	m_pManager = pManager;
	m_strKey = strKey;
	m_key = key;
}

bool_t Key::IsSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::KeyExists));
	request.write(m_key);
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
			WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
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
			WrongValueTypeException::Throw(strName,vtype,L"Omega::Registry::IRegistry::GetStringValue");
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
	if (!response->read(key))
		OMEGA_THROW(response->last_error());	
	
	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<Key> > ptrNew = ObjectImpl<Key>::CreateInstancePtr();

	ptrNew->Init(m_pManager,m_strKey + L"\\" + strSubKey,key);

	return ptrNew.AddRef();
}

Omega::IEnumString* Key::EnumSubKeys()
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::EnumSubKeys));
	request.write(m_key);
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

	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();
		
	try
	{
		for (;;)
		{
			std::string strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			ptrEnum->Append(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

Omega::IEnumString* Key::EnumValues()
{
	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::EnumValues));
	request.write(m_key);
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

	ObjectPtr<ObjectImpl<EnumString> > ptrEnum = ObjectImpl<EnumString>::CreateInstancePtr();
		
	try
	{
		for (;;)
		{
			std::string strName;
			if (!response->read(strName))
				OMEGA_THROW(response->last_error());

			if (strName.empty())
				break;

			ptrEnum->Append(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	ptrEnum->Init();
	return ptrEnum.AddRef();
}

void Key::DeleteKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	OOBase::CDRStream request;
	request.write(static_cast<Root::RootOpCode_t>(Root::DeleteKey));
	request.write(m_key);
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
