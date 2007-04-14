#include "OOServer.h"
#include ".\UserRegistry.h"
#include ".\UserManager.h"

using namespace Omega;
using namespace OTL;

void User::Registry::Base::Init(User::Manager* pManager)
{
	if (open_registry() != 0)
		OOSERVER_THROW_LASTERROR();

	m_ptrRootReg = ObjectImpl<Root>::CreateObjectPtr();
	m_ptrRootReg->Init(pManager);

	m_ptrUserReg = ObjectImpl<CurrentUser>::CreateObjectPtr();
	m_ptrUserReg->Init(m_registry.root_section());
}

int User::Registry::Base::open_registry()
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

	ACE_CString strRegistry = ACE_TEXT(OMEGA_REGISTRY_DIR "/" OMEGA_REGISTRY_FILE);

#endif

	return m_registry.open(strRegistry.c_str());
}

Omega::bool_t User::Registry::Base::IsSubKey(const Omega::string_t& key)
{
	if (key == "Current User")
		return true;
	else
		return m_ptrRootReg->IsSubKey(key);
}

Omega::bool_t User::Registry::Base::IsValue(const Omega::string_t& name)
{
	return m_ptrRootReg->IsValue(name);
}

Omega::string_t User::Registry::Base::GetStringValue(const Omega::string_t& name)
{
	return m_ptrRootReg->GetStringValue(name);
}

Omega::uint32_t User::Registry::Base::GetUIntValue(const Omega::string_t& name)
{
	return m_ptrRootReg->GetUIntValue(name);
}

void User::Registry::Base::GetBinaryValue(const Omega::string_t& name, Omega::uint32_t& cbLen, Omega::byte_t* pBuffer)
{
	return m_ptrRootReg->GetBinaryValue(name,cbLen,pBuffer);
}

void User::Registry::Base::SetStringValue(const Omega::string_t& name, const Omega::string_t& val)
{
	return m_ptrRootReg->SetStringValue(name,val);
}

void User::Registry::Base::SetUIntValue(const Omega::string_t& name, const Omega::uint32_t& val)
{
	return m_ptrRootReg->SetUIntValue(name,val);
}

void User::Registry::Base::SetBinaryValue(const Omega::string_t& name, Omega::uint32_t cbLen, const Omega::byte_t* val)
{
	return m_ptrRootReg->SetBinaryValue(name,cbLen,val);
}

Omega::Registry::IRegistryKey::ValueType_t User::Registry::Base::GetValueType(const Omega::string_t& name)
{
	return m_ptrRootReg->GetValueType(name);
}

Omega::Registry::IRegistryKey* User::Registry::Base::OpenSubKey(const Omega::string_t& key, Omega::Registry::IRegistryKey::OpenFlags_t flags)
{
	if (key.Left(9) == "Current User")
	{
		string_t sub_key = key.Mid(10);
		if (sub_key.IsEmpty())
		{
			if (flags & (Create | FailIfThere))
				OOSERVER_THROW_ERRNO(EEXIST);

			return m_ptrUserReg.AddRefReturn();
		}
		else
			return m_ptrUserReg->OpenSubKey(sub_key,flags);
	}
	else
	{
		return m_ptrRootReg->OpenSubKey(key,flags);
	}
}

Omega::IEnumString* User::Registry::Base::EnumSubKeys()
{
	void* TODO;

	return 0;
}

Omega::IEnumString* User::Registry::Base::EnumValues()
{
	return m_ptrRootReg->EnumValues();
}

void User::Registry::Base::DeleteKey(const Omega::string_t& strKey)
{
	if (strKey == "Current User")
		OOSERVER_THROW_ERRNO(EACCES);

	m_ptrRootReg->DeleteKey(strKey);
}

void User::Registry::Base::DeleteValue(const Omega::string_t& strValue)
{
	m_ptrRootReg->DeleteValue(strValue);
}
