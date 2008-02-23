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

		static void ValidateSubKey(const string_t& strSubKey, const string_t& strSource)
		{
			if (strSubKey.IsEmpty() ||
				strSubKey == L"\\" ||
				strSubKey.Right(1) == "\\" ||
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
				tp = "String";
			else if (actual_type==Integer)
				tp = "Integer";
			else if (actual_type==Binary)
				tp = "Binary";

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

void Key::Init(Manager* pManager, const Omega::string_t& strKey, const ACE_INT64& key)
{
	m_pManager = pManager;
	m_strKey = strKey;
	m_key = key;
}

bool_t Key::IsSubKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::KeyExists);
	request.write_longlong(m_key);
	request.write_string(strSubKey.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::IsSubKey");
	else if (err == ENOENT)
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

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::Octet value_type;
	if (err==0 && !response->read_octet(value_type))
		err = ACE_OS::last_error();

	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName,L"Omega::Registry::IRegistry::IsValue");
		else if (err==EACCES)
			AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::IsValue");
		else if (err != 0)
			OMEGA_THROW(err);
	}

	return (err==0);
}

ValueType_t Key::GetValueType(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetValueType");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::GetValueType");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetValueType");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetValueType");
	else if (err != 0)
		OMEGA_THROW(err);

	ACE_CDR::Octet value_type = 0;
	if (!response->read_octet(value_type))
		OOSERVER_THROW_LASTERROR();

	switch (value_type)
	{
	case 0:
		return String;

	case 1:
		return Integer;

	case 2:
		return Binary;

	default:
		OMEGA_THROW(EINVAL);
	}
}

string_t Key::GetStringValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetStringValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetStringValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::GetStringValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);

	ACE_CString strValue;
	if (!response->read_string(strValue))
		OOSERVER_THROW_LASTERROR();

	return string_t(strValue.c_str(),true);
}

int64_t Key::GetIntegerValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetIntegerValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetIntegerValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::GetIntegerValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);

	ACE_CDR::LongLong uValue = 0;
	if (!response->read_longlong(uValue))
		OOSERVER_THROW_LASTERROR();

	return uValue;
}

void Key::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::GetBinaryValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetBinaryValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	request.write_ulong(cbLen);
	bool bNoDataBack = (cbLen == 0);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::GetBinaryValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::GetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);

	if (!response->read_ulong(cbLen))
		OOSERVER_THROW_LASTERROR();

	if (!bNoDataBack && !response->read_octet_array(pBuffer,cbLen))
		OOSERVER_THROW_LASTERROR();
}

void Key::SetStringValue(const string_t& strName, const string_t& strValue)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetStringValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetStringValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	request.write_string(strValue.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::SetStringValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetStringValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetIntegerValue(const string_t& strName, const int64_t& value)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetIntegerValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetIntegerValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	request.write_longlong(value);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::SetIntegerValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetIntegerValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetIntegerValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::SetBinaryValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetBinaryValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	request.write_octet_array(val,cbLen);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName),L"Omega::Registry::IRegistry::SetBinaryValue");
		else
			NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::SetBinaryValue");
	else if (err != 0)
		OMEGA_THROW(err);
}

IRegistryKey* Key::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::CreateKey);
	request.write_longlong(m_key);
	request.write_string(strSubKey.ToUTF8().c_str());
	request.write_boolean((flags & IRegistryKey::Create) ? true : false);
	request.write_boolean((flags & IRegistryKey::FailIfThere) ? true : false);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
    *response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();
	
	if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==EALREADY)
		AlreadyExistsException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::OpenSubKey");
	else if (err != 0)
		OMEGA_THROW(err);

	ACE_INT64 key = 0;
    *response >> key;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();	
	
	// By the time we get here then we have successfully opened or created the key...
	ObjectPtr<ObjectImpl<Key> > ptrNew = ObjectImpl<Key>::CreateInstancePtr();

	ptrNew->Init(m_pManager,m_strKey + L"\\" + strSubKey,key);

	return ptrNew.AddRefReturn();
}

Omega::IEnumString* Key::EnumSubKeys()
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::EnumSubKeys);
	request.write_longlong(m_key);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumSubKeys");
	else if (err != 0)
		OMEGA_THROW(err);

	ObjectPtr<ObjectImpl<EnumSTL<Omega::IEnumString,string_t> > > ptrEnum = ObjectImpl<EnumSTL<Omega::IEnumString,string_t> >::CreateInstancePtr();
		
	try
	{
		for (;;)
		{
			ACE_CString strName;
			if (!response->read_string(strName))
				OOSERVER_THROW_LASTERROR();

			if (strName.empty())
				break;

			ptrEnum->Append(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	ptrEnum->Init();
	return ptrEnum.AddRefReturn();
}

Omega::IEnumString* Key::EnumValues()
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::EnumValues);
	request.write_longlong(m_key);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err==ENOENT)
		NotFoundException::Throw(m_strKey,L"Omega::Registry::IRegistry::EnumValues");
	else if (err != 0)
		OMEGA_THROW(err);

	ObjectPtr<ObjectImpl<EnumSTL<Omega::IEnumString,string_t> > > ptrEnum = ObjectImpl<EnumSTL<Omega::IEnumString,string_t> >::CreateInstancePtr();
		
	try
	{
		for (;;)
		{
			ACE_CString strName;
			if (!response->read_string(strName))
				OOSERVER_THROW_LASTERROR();

			if (strName.empty())
				break;

			ptrEnum->Append(string_t(strName.c_str(),true));
		}		
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	ptrEnum->Init();
	return ptrEnum.AddRefReturn();
}

void Key::DeleteKey(const string_t& strSubKey)
{
	BadNameException::ValidateSubKey(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteKey);
	request.write_longlong(m_key);
	request.write_string(strSubKey.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey + L"\\" + strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey,L"Omega::Registry::IRegistry::DeleteKey");
	else if (err != 0)
		OMEGA_THROW(err);
}

void Key::DeleteValue(const string_t& strName)
{
	BadNameException::ValidateValue(strName,L"Omega::Registry::IRegistry::DeleteValue");

	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteValue);
	request.write_longlong(m_key);
	request.write_string(strName.ToUTF8().c_str());
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_Refcounted_Auto_Ptr<ACE_InputCDR,ACE_Null_Mutex> response(m_pManager->sendrecv_root(request));
	if (response.null())
		OMEGA_THROW(EINVAL);

	int err = 0;
	*response >> err;
	if (!response->good_bit())
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EACCES)
		AccessDeniedException::Throw(m_strKey,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName,L"Omega::Registry::IRegistry::DeleteValue");
	else if (err != 0)
		OMEGA_THROW(err);
}
