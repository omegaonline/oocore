#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

class RegistryKeyImpl;

class Binding : public ACE_Naming_Context
{
public:
	ObjectPtr<ObjectImpl<RegistryKeyImpl> > m_ptrRootKey;	

	const ACE_TCHAR* name();
	const ACE_TCHAR* dll_name();
		
	Binding();
	virtual ~Binding();
};

class RegistryKeyImpl :
	public ObjectBase,
	public Registry::IRegistryKey
{
public:
	typedef ACE_DLL_Singleton_T<Binding, ACE_Thread_Mutex> BINDING;

	virtual ~RegistryKeyImpl()
	{ }

	bool_t IsSubKey(const string_t& key);
	bool_t IsValue(const string_t& name);
	string_t GetStringValue(const string_t& name);
	uint32_t GetUIntValue(const string_t& name);
	void GetBinaryValue(const string_t& name, uint32_t& cbLen, byte_t* pBuffer);
	void SetStringValue(const string_t& name, const string_t& val);
	void SetUIntValue(const string_t& name, const uint32_t& val);
	void SetBinaryValue(const string_t& name, uint32_t cbLen, const byte_t* val);
	IRegistryKey::ValueType_t GetValueType(const string_t& name);
	IRegistryKey* OpenSubKey(const string_t& key, OpenFlags_t flags = OpenExisting);
	Omega::IEnumString* EnumSubKeys();
	Omega::IEnumString* EnumValues();
	void DeleteKey(const string_t& strKey);
	void DeleteValue(const string_t& strKey);

	enum ValueType_Char
	{
		KEY	= 'K',
		VALUE_STRING = 'S',
		VALUE_UINT32 = 'U',
		VALUE_BINARY = 'B'
	};
	static bool get_value(const string_t& name, string_t& value, ValueType_Char& type);
	static void set_value(const string_t& name, const string_t& value, ValueType_Char type);
	static IRegistryKey::ValueType_t cast(ValueType_Char val);

	string_t m_strPath;

	BEGIN_INTERFACE_MAP(RegistryKeyImpl)
		INTERFACE_ENTRY(Registry::IRegistryKey)
	END_INTERFACE_MAP()
};

class EnumStringImpl : 
	public ObjectBase,
	public Omega::IEnumString
{
public:
	EnumStringImpl() : m_set(), m_iter(m_set.begin())
	{}

	ACE_PWSTRING_SET		m_set;
	ACE_PWSTRING_ITERATOR	m_iter;

	BEGIN_INTERFACE_MAP(EnumStringImpl)
		INTERFACE_ENTRY(IEnumString)
	END_INTERFACE_MAP()

// IEnumString members
public:
	void Next(uint32_t& count, string_t* parrVals);
	void Skip(uint32_t count);
	void Reset();
	Omega::IEnumString* Clone();
};

class WrongValueTypeExceptionImpl :
	public ExceptionImpl<Registry::IWrongValueTypeException>
{
public:
	BEGIN_INTERFACE_MAP(WrongValueTypeExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Registry::IWrongValueTypeException>)
	END_INTERFACE_MAP()

	Registry::IRegistryKey::ValueType_t m_type;

public:
	Registry::IRegistryKey::ValueType_t GetValueType()
	{
		return m_type;
	}

	static void Throw(RegistryKeyImpl::ValueType_Char actual_type, IException* pE = 0)
	{
		ObjectImpl<WrongValueTypeExceptionImpl>* pRE = ObjectImpl<WrongValueTypeExceptionImpl>::CreateObject();
		pRE->m_type = RegistryKeyImpl::cast(actual_type);
		pRE->m_ptrCause = pE;
		pRE->m_strDesc = "Incorrect registry value type, actual value type is ";
		if (actual_type==RegistryKeyImpl::VALUE_STRING)
			pRE->m_strDesc += "String";
		else if (actual_type==RegistryKeyImpl::VALUE_UINT32)
			pRE->m_strDesc += "UInt32";
		else if (actual_type==RegistryKeyImpl::VALUE_BINARY)
			pRE->m_strDesc += "Binary";
		else
			pRE->m_strDesc += "Corrupt!";
		throw pRE;
	}
};

class NotFoundExceptionImpl :
	public ExceptionImpl<Registry::INotFoundException>
{
public:
	BEGIN_INTERFACE_MAP(NotFoundExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Registry::INotFoundException>)
	END_INTERFACE_MAP()

	string_t m_strName;

public:
	string_t GetValueName()
	{
		return m_strName;
	}

	static void Throw(const string_t& name, IException* pE = 0)
	{
		ObjectImpl<NotFoundExceptionImpl>* pRE = ObjectImpl<NotFoundExceptionImpl>::CreateObject();
		pRE->m_strName = name;
		pRE->m_ptrCause = pE;
		pRE->m_strDesc = "Value '" + name + "' not found.";
		throw pRE;
	}
};

class BadNameExceptionImpl :
	public ExceptionImpl<Registry::IBadNameException>
{
public:
	BEGIN_INTERFACE_MAP(BadNameExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Registry::IBadNameException>)
	END_INTERFACE_MAP()

	string_t m_strName;

public:
	string_t GetName()
	{
		return m_strName;
	}

	static void Throw(const string_t& name, IException* pE = 0)
	{
		ObjectImpl<BadNameExceptionImpl>* pRE = ObjectImpl<BadNameExceptionImpl>::CreateObject();
		pRE->m_strName = name;
		pRE->m_ptrCause = pE;
		pRE->m_strDesc = "Invalid name for registry key or value: '" + name + "'.";
		throw pRE;
	}
};

class AlreadyExistsExceptionImpl :
	public ExceptionImpl<Registry::IAlreadyExistsException>
{
public:
	BEGIN_INTERFACE_MAP(AlreadyExistsExceptionImpl)
		INTERFACE_ENTRY_CHAIN(ExceptionImpl<Registry::IAlreadyExistsException>)
	END_INTERFACE_MAP()

	string_t m_strName;

public:
	string_t GetKeyName()
	{
		return m_strName;
	}

	static void Throw(const string_t& name, IException* pE = 0)
	{
		ObjectImpl<AlreadyExistsExceptionImpl>* pRE = ObjectImpl<AlreadyExistsExceptionImpl>::CreateObject();
		pRE->m_strName = name;
		pRE->m_ptrCause = pE;
		pRE->m_strDesc = "Key '" + name + "' already exists.";
		throw pRE;
	}
};

// EnumStringImpl
void EnumStringImpl::Next(uint32_t& count, string_t* parrVals)
{
	uint32_t c;
	for (c=0;c<count && !m_iter.done();++c)
		parrVals[c] = ACE_Wide_To_Ascii((*(m_iter++)).c_str()).char_rep();
	
	count = c;
}

void EnumStringImpl::Skip(uint32_t count)
{
	for (uint32_t c=0;c<count && !m_iter.done();++c)
		m_iter.advance();
}

void EnumStringImpl::Reset()
{
	m_iter.first();
}

Omega::IEnumString* EnumStringImpl::Clone()
{
	ObjectPtr<ObjectImpl<EnumStringImpl> > ptrEnum(ObjectImpl<EnumStringImpl>::CreateObjectPtr());

	ptrEnum->m_set = m_set;
	ptrEnum->m_iter = ptrEnum->m_set.begin();

	return ptrEnum.Detach();
}

// RegistryKeyImpl
bool RegistryKeyImpl::get_value(const string_t& name, string_t& value, ValueType_Char& type)
{
	ACE_NS_WString strValue;
	char* type_out = 0;
	if (BINDING::instance()->resolve(ACE_NS_WString(name),strValue,type_out) != 0)
	{
		// Resolve can fail on a out of memory or 'cos the key is not found
		if (ACE_OS::last_error() != ENOENT)
			OOCORE_THROW_LASTERROR();

		return false;
	}
	type = static_cast<ValueType_Char>(type_out[0]);
	delete [] type_out;

	if (type != KEY &&
		type != VALUE_STRING &&
		type != VALUE_UINT32 &&
		type != VALUE_BINARY)
	{
		OMEGA_THROW("Invalid binding type");
	}
	
	value = ACE_Wide_To_Ascii(strValue.c_str()).char_rep();

	return true;
}

void RegistryKeyImpl::set_value(const string_t& name, const string_t& value, ValueType_Char type)
{
	char type_out[2] = {0};
	type_out[0] = static_cast<char>(type);
	
    if (BINDING::instance()->rebind(ACE_NS_WString(name),ACE_NS_WString(value),type_out) < 0)
		OOCORE_THROW_LASTERROR();
}

bool_t RegistryKeyImpl::IsSubKey(const string_t& key)
{
	string_t value;
	ValueType_Char type;

	if (key.IsEmpty())
		BadNameExceptionImpl::Throw(key);

	return RegistryKeyImpl::get_value(m_strPath + key + "/",value,type);
}

bool_t RegistryKeyImpl::IsValue(const string_t& name)
{
	string_t value;
	ValueType_Char type;

	if (name.IsEmpty())
		BadNameExceptionImpl::Throw(name);

	return RegistryKeyImpl::get_value(m_strPath + name,value,type);
}

string_t RegistryKeyImpl::GetStringValue(const string_t& name)
{
	string_t value;
	ValueType_Char type;
	
	if (name.IsEmpty())
		BadNameExceptionImpl::Throw(name);

	if (!RegistryKeyImpl::get_value(m_strPath + name,value,type))
		NotFoundExceptionImpl::Throw(m_strPath + name);
	
	if (type != RegistryKeyImpl::VALUE_STRING)
		WrongValueTypeExceptionImpl::Throw(type);
		
	return value;
}

uint32_t RegistryKeyImpl::GetUIntValue(const string_t& name)
{
	string_t value;
	ValueType_Char type;

	if (name.IsEmpty())
		BadNameExceptionImpl::Throw(name);

	if (!RegistryKeyImpl::get_value(m_strPath + name,value,type))
		NotFoundExceptionImpl::Throw(m_strPath + name);

	if (type != RegistryKeyImpl::VALUE_UINT32)
		WrongValueTypeExceptionImpl::Throw(type);

	// Just check we are not running on some very bizarre platform
	#if ACE_SIZEOF_LONG < 4
		#error Need to write an alternate routine for your platform!
	#endif

	char_t* end_ptr = 0;
	unsigned long val = ACE_OS::strtoul(value,&end_ptr,10);

	if (*end_ptr != '\0')
		WrongValueTypeExceptionImpl::Throw(RegistryKeyImpl::VALUE_STRING);

	return static_cast<uint32_t>(val);
}

void RegistryKeyImpl::GetBinaryValue(const string_t& name, uint32_t& cbLen, byte_t* pBuffer)
{
	string_t value;
	ValueType_Char type;
	
	if (name.IsEmpty())
		BadNameExceptionImpl::Throw(name);

	if (!RegistryKeyImpl::get_value(m_strPath + name,value,type))
		NotFoundExceptionImpl::Throw(m_strPath + name);
	
	if (type != RegistryKeyImpl::VALUE_BINARY)
		WrongValueTypeExceptionImpl::Throw(type);

	// This is not an efficient way to store the data - do not make your binary data entries too big!
	if (pBuffer == 0)
		cbLen = static_cast<uint32_t>(ACE_Base64::length(reinterpret_cast<const ACE_Byte*>(static_cast<const char_t*>(value))));
	else
	{
		ACE_Byte* pData = ACE_Base64::decode(reinterpret_cast<const ACE_Byte*>(static_cast<const char_t*>(value)),&cbLen);
		if (!pData)
			OMEGA_THROW("Bad base64 format");
				
		memcpy(pBuffer,pData,cbLen);
		delete [] pData;
	}
}

void RegistryKeyImpl::SetStringValue(const string_t& name, const string_t& val)
{
	// Check for a valid name
	if (name.IsEmpty() || name.Find('/') != -1)
		BadNameExceptionImpl::Throw(name);

	set_value(m_strPath + name,val,VALUE_STRING);
}

void RegistryKeyImpl::SetUIntValue(const string_t& name, const uint32_t& val)
{
	// Check for a valid name
	if (name.IsEmpty() || name.Find('/') != -1)
		BadNameExceptionImpl::Throw(name);

	ACE_TCHAR szBuf[32] = {0};
	if (ACE_OS::snprintf(szBuf,31,ACE_UINT32_FORMAT_SPECIFIER,val)<=0)
		OMEGA_THROW("Bizzare response from snprintf");
		
	set_value(m_strPath + name,ACE_TEXT_ALWAYS_CHAR(szBuf),VALUE_UINT32);
}

void RegistryKeyImpl::SetBinaryValue(const string_t& name, uint32_t cbLen, const byte_t* val)
{
	// Check for a valid name
	if (name.IsEmpty() || name.Find('/') != -1)
		BadNameExceptionImpl::Throw(name);

	size_t len = 0;
	ACE_Byte* buf = ACE_Base64::encode(static_cast<const ACE_Byte*>(val),cbLen,&len);
	if (!buf && cbLen)
		OOCORE_THROW_LASTERROR();

	// Protect against throwing with an auto_ptr
	ACE_Auto_Basic_Array_Ptr<ACE_Byte> buffer(buf);

	// This is not an efficient way to store the data - do not make your binary data entries too big!
    set_value(m_strPath + name,reinterpret_cast<char_t*>(buf),VALUE_BINARY);
}

Registry::IRegistryKey::ValueType_t RegistryKeyImpl::cast(ValueType_Char val)
{
	switch (val)
	{
	case RegistryKeyImpl::VALUE_STRING:
		return IRegistryKey::String;
	
	case RegistryKeyImpl::VALUE_UINT32:
		return IRegistryKey::UInt32;
	
	case RegistryKeyImpl::VALUE_BINARY:
		return IRegistryKey::Binary;
    
	case RegistryKeyImpl::KEY:
	default:
		OMEGA_THROW("Corrupt binding type");
	};
		
	return IRegistryKey::Binary;
}

Registry::IRegistryKey::ValueType_t RegistryKeyImpl::GetValueType(const string_t& name)
{
	string_t value;
	ValueType_Char type;
	
	if (!RegistryKeyImpl::get_value(m_strPath + name,value,type))
		NotFoundExceptionImpl::Throw(m_strPath + name);
	
	return cast(type);
}

Registry::IRegistryKey* RegistryKeyImpl::OpenSubKey(const string_t& key, OpenFlags_t flags)
{
	ObjectImpl<RegistryKeyImpl>* pKey = 0;

	// Parse down path, checking keys along the way
	size_t pos = (size_t)-1, next = 0;
	do
	{
		string_t full_key;
		next = key.Find('/',pos+1);
		if (next != -1)
		{
			// Check for //
			if (next == pos+1)
				BadNameExceptionImpl::Throw(key);

			full_key = m_strPath + key.Left(next+1);
			pos = next;
		}
		else
		{
			// Check for trailing /
			if (pos+1 == static_cast<ssize_t>(key.Length()))
				BadNameExceptionImpl::Throw(key);

			full_key = m_strPath + key + "/";
		}
					
		// See if it exists so far
		string_t value;
		ValueType_Char type;
		if (RegistryKeyImpl::get_value(full_key,value,type))
		{
			if (type != RegistryKeyImpl::KEY)
				OMEGA_THROW("Corrupt binding type");

			if (next == -1)
			{
				// We have the last part of the path - check we can overwrite it...
				if (flags & IRegistryKey::FailIfThere)
					AlreadyExistsExceptionImpl::Throw(full_key.Mid(0,static_cast<ssize_t>(full_key.Length())-1));
			}
		}
		else
		{
			// Key doesn't exist - check we are allowed to create
			if (!(flags & IRegistryKey::Create))
				NotFoundExceptionImpl::Throw(full_key.Mid(0,static_cast<ssize_t>(full_key.Length())-1));

			// Create parent paths as we descend
			RegistryKeyImpl::set_value(full_key,"",RegistryKeyImpl::KEY);
		}

	} while (next != -1);

	// Create the object
	pKey = ObjectImpl<RegistryKeyImpl>::CreateObject();
	pKey->m_strPath = m_strPath + key + "/";
		
	return pKey;
}

Omega::IEnumString* RegistryKeyImpl::EnumSubKeys()
{
	ACE_BINDING_SET name_set;
	if (BINDING::instance()->list_name_entries(name_set,ACE_NS_WString(m_strPath)) < 0)
		OOCORE_THROW_LASTERROR();

	ObjectPtr<ObjectImpl<EnumStringImpl> > ptrEnum(ObjectImpl<EnumStringImpl>::CreateObjectPtr());

	for (ACE_BINDING_ITERATOR i=name_set.begin(); i.done()==0; i.advance())
	{
		if ((*i).type_[0] == KEY)
		{
			size_t pos = (*i).name_.find('/',m_strPath.Length());
			if (pos == static_cast<size_t>((*i).name_.length())-1)
			{
				if (ptrEnum->m_set.insert((*i).name_.substr(m_strPath.Length(),pos-(ssize_t)m_strPath.Length()).c_str()) == -1)
					OOCORE_THROW_LASTERROR();
			}
		}
	}	

	ptrEnum->m_iter = ptrEnum->m_set.begin();

	return ptrEnum.Detach();
}

Omega::IEnumString* RegistryKeyImpl::EnumValues()
{
	ACE_BINDING_SET name_set;
	if (BINDING::instance()->list_name_entries(name_set,ACE_NS_WString(m_strPath)) != 0)
		OOCORE_THROW_LASTERROR();

	ObjectPtr<ObjectImpl<EnumStringImpl> > ptrEnum(ObjectImpl<EnumStringImpl>::CreateObjectPtr());

	for (ACE_BINDING_ITERATOR i=name_set.begin(); i.done()==0; i.advance())
	{
		if ((*i).type_[0] != KEY)
		{
			if ((*i).name_.find('/',m_strPath.Length())==ACE_WString::npos)
			{
				if (ptrEnum->m_set.insert((*i).name_.substr(m_strPath.Length()).c_str()) == -1)
					OOCORE_THROW_LASTERROR();
			}
		}
	}

	ptrEnum->m_iter = ptrEnum->m_set.begin();

	return ptrEnum.Detach();
}

void RegistryKeyImpl::DeleteKey(const string_t& strKey)
{
	ACE_NS_WString strFullPath(m_strPath + strKey + "/");

	ACE_PWSTRING_SET name_set;
	int res = BINDING::instance()->list_names(name_set,strFullPath);
	if (res < 0)
		OOCORE_THROW_LASTERROR();
	else if (res==1)
		NotFoundExceptionImpl::Throw(m_strPath + strKey);

	for (ACE_PWSTRING_ITERATOR i=name_set.begin(); i.done()==0; i.advance())
	{
		if (BINDING::instance()->unbind(*i) != 0)
		{
			// Resolve can fail on a out of memory or 'cos the key is not found
			if (ACE_OS::last_error() != ENOENT)
				OOCORE_THROW_LASTERROR();

			NotFoundExceptionImpl::Throw(m_strPath + strKey);
		}
	}
}

void RegistryKeyImpl::DeleteValue(const string_t& strValue)
{
	if (BINDING::instance()->unbind(ACE_NS_WString(m_strPath + strValue)) != 0)
	{
		// Resolve can fail on a out of memory or 'cos the key is not found
		if (ACE_OS::last_error() != ENOENT)
			OOCORE_THROW_LASTERROR();

		NotFoundExceptionImpl::Throw(m_strPath + strValue);
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Registry::IRegistryKey*,IRegistryKey_OpenKey,2,((in),const string_t&,key,(in),Registry::IRegistryKey::OpenFlags_t,flags))
{
	if (key == "/")
		return RegistryKeyImpl::BINDING::instance()->m_ptrRootKey.AddRefReturn();

	// Open a sub key on the root key
	return RegistryKeyImpl::BINDING::instance()->m_ptrRootKey->OpenSubKey(key,flags);	
}

// Exceptions
/*

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(IRegistryKey_INotFoundException_Throw,2,(const string_t& strName, IException* pE),(strName,pE))
{
	ObjectImpl<NotFoundExceptionImpl>* pRE = ObjectImpl<NotFoundExceptionImpl>::CreateObject();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format("Registry key '%s' not found",static_cast<const char_t*>(strName));
	throw pRE;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(IRegistryKey_IBadNameException_Throw,2,(const string_t& strName, IException* pE),(strName,pE))
{
	ObjectImpl<BadNameExceptionImpl>* pRE = ObjectImpl<BadNameExceptionImpl>::CreateObject();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format("%s is an invalid name for a registry key or value",static_cast<const char_t*>(strName));
	throw pRE;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(IRegistryKey_IAlreadyExistsException_Throw,2,(const string_t& strKey, IException* pE),(strKey,pE))
{
	ObjectImpl<AlreadyExistsExceptionImpl>* pRE = ObjectImpl<AlreadyExistsExceptionImpl>::CreateObject();
	pRE->m_ptrCause = pE;
	pRE->m_strDesc = string_t::Format("Registry key '%s' already exists",static_cast<const char_t*>(strKey));
	throw pRE;
}*/

// Binding
Binding::Binding() : 
	m_ptrRootKey(ObjectImpl<RegistryKeyImpl>::CreateObjectPtr())
{
	name_options()->database(ACE_TEXT("OmegaOnline.reg_db"));
	
#if defined(ACE_WIN32)
	ACE_TCHAR szBuf[MAX_PATH] = {0};
	if (::SHGetSpecialFolderPath(0,szBuf,CSIDL_COMMON_APPDATA,0))
	{
		::PathAppend(szBuf,"OmegaOnline");
		if (!::PathFileExists(szBuf))
		{
			if (ACE_OS::mkdir(szBuf) != 0)
				goto errored;
		}
		else if (!::PathIsDirectory(szBuf))
			goto trylocal;
	}
	else
	{
trylocal:
		if (ACE_TEXT_GetModuleFileName(0,szBuf,MAX_PATH)!=0)
		{
			::PathRemoveFileSpec(szBuf);
		}
	}
	
	if (szBuf[0] == 0)
	{
errored:
		string_t strMsg("Can't find registry database '");
		OMEGA_THROW(strMsg + ACE_TEXT_ALWAYS_CHAR(szBuf) + "'");
	}

	name_options()->namespace_dir(szBuf);

#if (defined (ACE_HAS_WINNT4) && ACE_HAS_WINNT4 != 0)

	// Use a different base address
	name_options()->base_address((char*)(1024UL*1024*512));
	
	// Sometimes the base address is already in use - .NET CLR for example
	// So we check it first - I wish ACE would do this for us!
	// The problem with just defaulting to address 0x0 - which mean pick any,
	// is that ACE seems to crash creating the MEM_Map for the first time!
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(name_options()->base_address(),&mbi,sizeof(mbi)))
	{
		if (mbi.State != MEM_FREE)
		{
			// Please record which addresses aren't useful!, e.g.
			//
			// 0x04000000	-	Used by VB.NET
			//
			ACE_OS::abort();
		}
	}	
#endif

#else // ACE_WIN32
	#warning Need to sort this out under *NIX!
	// HUGE HACK TO GET THIS WORKING UNDER *NIX
	name_options()->namespace_dir(ACE_TEXT("/tmp/"));
#endif

	name_options()->context(ACE_Naming_Context::NODE_LOCAL);

	if (open(name_options()->context(),0) != 0)
		OOCORE_THROW_LASTERROR();

	m_ptrRootKey->m_strPath = "/";
}

Binding::~Binding()
{
}

const ACE_TCHAR* Binding::name()
{
	return ACE_TEXT("Binding");
}

const ACE_TCHAR* Binding::dll_name()
{
	return ACE_TEXT("OOCore");
}
