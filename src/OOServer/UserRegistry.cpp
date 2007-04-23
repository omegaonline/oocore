#include "OOServer.h"
#include ".\UserRegistry.h"
#include ".\UserManager.h"

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

		static void Throw(const string_t& name, IException* pE = 0)
		{
			ObjectImpl<BadNameException>* pRE = ObjectImpl<BadNameException>::CreateObject();
			pRE->m_strName = name;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = "Invalid name for registry key or value: '" + name + "'.";
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

		IRegistryKey::ValueType_t m_type;

	public:
		IRegistryKey::ValueType_t GetValueType()
		{
			return m_type;
		}

		static void Throw(string_t strValue, IRegistryKey::ValueType_t actual_type, IException* pE = 0)
		{
			ObjectImpl<WrongValueTypeException>* pRE = ObjectImpl<WrongValueTypeException>::CreateObject();
			pRE->m_type = actual_type;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = "Incorrect registry value type, actual value type is ";
			if (actual_type==IRegistryKey::String)
				pRE->m_strDesc += "String";
			else if (actual_type==IRegistryKey::UInt32)
				pRE->m_strDesc += "UInt32";
			else if (actual_type==IRegistryKey::Binary)
				pRE->m_strDesc += "Binary";
			else
				pRE->m_strDesc += "Corrupt!";
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
		string_t GetValueName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, IException* pE = 0)
		{
			ObjectImpl<NotFoundException>* pRE = ObjectImpl<NotFoundException>::CreateObject();
			pRE->m_strName = name;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = "Value '" + name + "' not found.";
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

		static void Throw(const string_t& name, IException* pE = 0)
		{
			ObjectImpl<AlreadyExistsException>* pRE = ObjectImpl<AlreadyExistsException>::CreateObject();
			pRE->m_strName = name;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = "Key '" + name + "' already exists.";
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
		string_t GetName()
		{
			return m_strName;
		}

		static void Throw(const string_t& name, IException* pE = 0)
		{
			ObjectImpl<AccessDeniedException>* pRE = ObjectImpl<AccessDeniedException>::CreateObject();
			pRE->m_strName = name;
			pRE->m_ptrCause = pE;
			pRE->m_strDesc = "Write attempt illegal for '" + name + "'.";
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

void UserKey::Init(ACE_Configuration_Heap* pRegistry, const ACE_Configuration_Section_Key& strKey, ACE_Thread_Mutex* pLock)
{
	m_pRegistry = pRegistry;
	m_key = strKey;
	m_pLock = pLock;
}

bool_t UserKey::IsSubKey(const string_t& strSubKey)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	ACE_Configuration_Section_Key sub_key;
	if (m_pRegistry->open_section(m_key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey),0,sub_key) == 0)
		return true;

	int err = ACE_OS::last_error();
	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey);
		else
			OOSERVER_THROW_ERRNO(err);
	}
	return false;
}

bool_t UserKey::IsValue(const string_t& strName)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	ACE_Configuration_Heap::VALUETYPE vtype;
	if (m_pRegistry->find_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),vtype) == 0)
		return true;

	int err = ACE_OS::last_error();
	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}
	return false;
}

string_t UserKey::GetStringValue(const string_t& strName)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	ACE_TString strValue;
	if (m_pRegistry->get_string_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),strValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(strName,GetValueType(strName));
			else
				NotFoundException::Throw(strName);
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return string_t(ACE_TEXT_ALWAYS_CHAR(strValue.c_str()));
}

uint32_t UserKey::GetUIntValue(const string_t& strName)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	u_int uValue = 0;
	if (m_pRegistry->get_integer_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),uValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(strName,GetValueType(strName));
			else
				NotFoundException::Throw(strName);
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return static_cast<uint32_t>(uValue);
}

void UserKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	::DebugBreak();
	void* TODO;
}

void UserKey::SetStringValue(const string_t& strName, const string_t& val)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	ACE_TString strValue(ACE_TEXT_CHAR_TO_TCHAR(val));
	if (m_pRegistry->set_string_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),strValue) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(strName,GetValueType(strName));
			else
				NotFoundException::Throw(strName);
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::SetUIntValue(const string_t& strName, const uint32_t& val)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	if (m_pRegistry->set_integer_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),val) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(strName,GetValueType(strName));
			else
				NotFoundException::Throw(strName);
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	::DebugBreak();
	void* TODO;
}

IRegistryKey::ValueType_t UserKey::GetValueType(const string_t& strName)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	ACE_Configuration_Heap::VALUETYPE vtype;
	if (m_pRegistry->find_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName),vtype) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
		{
			if (IsValue(strName))
				WrongValueTypeException::Throw(strName,GetValueType(strName));
			else
				NotFoundException::Throw(strName);
		}
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	switch (vtype)
	{
	case ACE_Configuration_Heap::STRING:
		return IRegistryKey::String;

	case ACE_Configuration_Heap::INTEGER:
		return IRegistryKey::UInt32;

	case ACE_Configuration_Heap::BINARY:
		return IRegistryKey::Binary;

	default:
		OOSERVER_THROW_ERRNO(EINVAL);
		return static_cast<IRegistryKey::ValueType_t>(-1);
	}
}

IRegistryKey* UserKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	if (flags & IRegistryKey::FailIfThere)
	{
		// Check to see if the key already exists by opening it...
		ACE_Configuration_Section_Key sub_key;
		if (m_pRegistry->open_section(m_key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey),0,sub_key) == 0)
			AlreadyExistsException::Throw(strSubKey);
	}

    int bCreate = (flags & IRegistryKey::Create) ? 1 : 0;

	ACE_Configuration_Section_Key sub_key;
	if (m_pRegistry->open_section(m_key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey),bCreate,sub_key) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(strSubKey);
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	// If we get here, then we have a new key!
	ObjectPtr<ObjectImpl<UserKey> > ptrSubKey = ObjectImpl<UserKey>::CreateObjectPtr();
	ptrSubKey->Init(m_pRegistry,sub_key,m_pLock);
	return ptrSubKey.AddRefReturn();
}

Omega::IEnumString* UserKey::EnumSubKeys()
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	std::set<string_t> setSubKeys;
	for (int index=0;;++index)
	{
		ACE_TString strSubKey;
		int err = m_pRegistry->enumerate_sections(m_key,index,strSubKey);
		if (err == 0)
			setSubKeys.insert(ACE_TEXT_ALWAYS_CHAR(strSubKey.c_str()));
		else if (err == 1)
			break;
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return EnumString::Create(setSubKeys.begin(),setSubKeys.end());
}

Omega::IEnumString* UserKey::EnumValues()
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	std::set<string_t> setValues;
	for (int index=0;;++index)
	{
		ACE_TString strValue;
		ACE_Configuration_Heap::VALUETYPE type;
		int err = m_pRegistry->enumerate_values(m_key,index,strValue,type);
		if (err == 0)
			setValues.insert(ACE_TEXT_ALWAYS_CHAR(strValue.c_str()));
		else if (err == 1)
			break;
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return EnumString::Create(setValues.begin(),setValues.end());
}

void UserKey::DeleteKey(const string_t& strSubKey)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	if (m_pRegistry->remove_section(m_key,ACE_TEXT_CHAR_TO_TCHAR(strSubKey),1) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(strSubKey);
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey);
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void UserKey::DeleteValue(const string_t& strName)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,*m_pLock,OOSERVER_THROW_LASTERROR());

	if (m_pRegistry->remove_value(m_key,ACE_TEXT_CHAR_TO_TCHAR(strName)) != 0)
	{
		int err = ACE_OS::last_error();
		if (err == ENOENT)
			NotFoundException::Throw(strName);
		else if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}
}

void RootKey::Init(Manager* pManager, const string_t& strKey)
{
	m_pManager = pManager;
	m_strKey = strKey;
}

bool_t RootKey::IsSubKey(const string_t& strSubKey)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::KeyExists);
	request.write_string(m_strKey);
	request.write_string(strSubKey);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::Boolean bRes = false;
	if (err==0 && !response.read_boolean(bRes))
		err = ACE_OS::last_error();

	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strSubKey);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return bRes;
}

bool_t RootKey::IsValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_string(m_strKey);
	request.write_string(strName);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::Octet value_type;
	if (err==0 && !response.read_octet(value_type))
		err = ACE_OS::last_error();

	if (err != ENOENT)
	{
		if (err==EINVAL || err==ENAMETOOLONG)
			BadNameException::Throw(strName);
		else
			OOSERVER_THROW_ERRNO(err);
	}

	return (err==0);
}

IRegistryKey::ValueType_t RootKey::GetValueType(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::ValueType);
	request.write_string(m_strKey);
	request.write_string(strName);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::Octet value_type = 0;
	if (err==0 && !response.read_octet(value_type))
		err = ACE_OS::last_error();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName));
		else
			NotFoundException::Throw(strName);
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);

	switch (value_type)
	{
	case ACE_Configuration_Heap::STRING:
		return IRegistryKey::String;

	case ACE_Configuration_Heap::INTEGER:
		return IRegistryKey::UInt32;

	case ACE_Configuration_Heap::BINARY:
		return IRegistryKey::Binary;

	default:
		OOSERVER_THROW_ERRNO(EINVAL);
		return static_cast<IRegistryKey::ValueType_t>(-1);
	}
}

string_t RootKey::GetStringValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetStringValue);
	request.write_string(m_strKey);
	request.write_string(strName);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	ACE_CString strValue;
	if (err==0 && !response.read_string(strValue))
		err = ACE_OS::last_error();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName));
		else
			NotFoundException::Throw(strName);
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err!=0)
		OOSERVER_THROW_ERRNO(err);

	return strValue.c_str();
}

uint32_t RootKey::GetUIntValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::GetUInt32Value);
	request.write_string(m_strKey);
	request.write_string(strName);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	ACE_CDR::ULong uValue = 0;
	if (err==0 && !response.read_ulong(uValue))
		err = ACE_OS::last_error();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName));
		else
			NotFoundException::Throw(strName);
	}
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err!=0)
		OOSERVER_THROW_ERRNO(err);

	return uValue;
}

void RootKey::GetBinaryValue(const Omega::string_t& strName, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	::DebugBreak();
	void* TODO;
}

void RootKey::SetStringValue(const string_t& strName, const string_t& strValue)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetStringValue);
	request.write_string(m_strKey);
	request.write_string(strName);
	request.write_string(strValue);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName));
		else
			NotFoundException::Throw(strName);
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(strName);
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::SetUIntValue(const string_t& strName, const uint32_t& uValue)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::SetUInt32Value);
	request.write_string(m_strKey);
	request.write_string(strName);
	request.write_ulong(uValue);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
	{
		if (IsValue(strName))
			WrongValueTypeException::Throw(strName,GetValueType(strName));
		else
			NotFoundException::Throw(strName);
	}
	else if (err==EACCES)
		AccessDeniedException::Throw(strName);
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::SetBinaryValue(const Omega::string_t& strName, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	::DebugBreak();
	void* TODO;
}

IRegistryKey* RootKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	::DebugBreak();
	void* TODO;
	return 0;
}

Omega::IEnumString* RootKey::EnumSubKeys()
{
	::DebugBreak();
	void* TODO;
	return 0;
}

void RootKey::EnumSubKeys(std::set<Omega::string_t>& setStrings)
{
	::DebugBreak();
	void* TODO;
}

Omega::IEnumString* RootKey::EnumValues()
{
	std::set<Omega::string_t> setStrings;
	EnumSubKeys(setStrings);

	return EnumString::Create(setStrings.begin(),setStrings.end());
}

void RootKey::DeleteKey(const string_t& strSubKey)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteKey);
	request.write_string(m_strKey);
	request.write_string(strSubKey);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(strSubKey);
	else if (err==EACCES)
		AccessDeniedException::Throw(strSubKey);
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strSubKey);
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void RootKey::DeleteValue(const string_t& strName)
{
	ACE_OutputCDR request;
	request << static_cast<Root::RootOpCode_t>(Root::DeleteValue);
	request.write_string(m_strKey);
	request.write_string(strName);
	if (!request.good_bit())
		OOSERVER_THROW_LASTERROR();

	ACE_InputCDR response = m_pManager->send_synch(ACE_INVALID_HANDLE,0,request.begin());

	ACE_CDR::ULong err = 0;
	if (!response.read_ulong(err))
		OOSERVER_THROW_LASTERROR();

	if (err == ENOENT)
		NotFoundException::Throw(strName);
	else if (err==EACCES)
		AccessDeniedException::Throw(strName);
	else if (err==EINVAL || err==ENAMETOOLONG)
		BadNameException::Throw(strName);
	else if (err != 0)
		OOSERVER_THROW_ERRNO(err);
}

void BaseKey::Init(Manager* pManager)
{
	if (open_registry() != 0)
		OOSERVER_THROW_LASTERROR();

	m_ptrRoot = ObjectImpl<RootKey>::CreateObjectPtr();
	m_ptrRoot->Init(pManager,"");

	m_ptrUser = ObjectImpl<UserKey>::CreateObjectPtr();
	m_ptrUser->Init(&m_registry,m_registry.root_section(),&m_lock);
}

int BaseKey::open_registry()
{
#define OMEGA_REGISTRY_FILE "user.regdb"

#if defined(ACE_WIN32)

	ACE_CString strRegistry = "C:\\" OMEGA_REGISTRY_FILE;

	char szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathA(0,CSIDL_LOCAL_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if SUCCEEDED(hr)
	{
		char szBuf2[MAX_PATH] = {0};
		if (PathCombineA(szBuf2,szBuf,"OmegaOnline"))
		{
			if (!PathFileExistsA(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					return ret;
			}

			if (PathCombineA(szBuf,szBuf2,OMEGA_REGISTRY_FILE))
				strRegistry = szBuf;
		}
	}

#else

#error Fix me!

#define OMEGA_REGISTRY_DIR "/var/lib/OmegaOnline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			return -1;
	}

	ACE_CString strRegistry = OMEGA_REGISTRY_DIR "/" OMEGA_REGISTRY_FILE;

#endif

	return m_registry.open(strRegistry.c_str());
}

bool_t BaseKey::IsSubKey(const string_t& strSubKey)
{
	if (strSubKey == "Current User")
		return true;
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

void BaseKey::SetUIntValue(const string_t& strName, const uint32_t& val)
{
	return m_ptrRoot->SetUIntValue(strName,val);
}

void BaseKey::SetBinaryValue(const string_t& strName, uint32_t cbLen, const byte_t* val)
{
	return m_ptrRoot->SetBinaryValue(strName,cbLen,val);
}

IRegistryKey::ValueType_t BaseKey::GetValueType(const string_t& strName)
{
	return m_ptrRoot->GetValueType(strName);
}

IRegistryKey* BaseKey::OpenSubKey(const string_t& strSubKey, IRegistryKey::OpenFlags_t flags)
{
	if (strSubKey.Left(12) == "Current User")
	{
		string_t sub_strSubKey = strSubKey.Mid(13);
		if (sub_strSubKey.IsEmpty())
		{
			if (flags & IRegistryKey::FailIfThere)
				OOSERVER_THROW_ERRNO(EEXIST);

			return m_ptrUser.AddRefReturn();
		}
		else
			return m_ptrUser->OpenSubKey(sub_strSubKey,flags);
	}
	else
	{
		return m_ptrRoot->OpenSubKey(strSubKey,flags);
	}
}

Omega::IEnumString* BaseKey::EnumSubKeys()
{
	std::set<string_t>	setStrings;
	m_ptrRoot->EnumSubKeys(setStrings);

	setStrings.insert("Current User");

	return EnumString::Create(setStrings.begin(),setStrings.end());
}

Omega::IEnumString* BaseKey::EnumValues()
{
	return m_ptrRoot->EnumValues();
}

void BaseKey::DeleteKey(const string_t& strSubKey)
{
	if (strSubKey == "Current User")
		OOSERVER_THROW_ERRNO(EACCES);

	m_ptrRoot->DeleteKey(strSubKey);
}

void BaseKey::DeleteValue(const string_t& strName)
{
	m_ptrRoot->DeleteValue(strName);
}
