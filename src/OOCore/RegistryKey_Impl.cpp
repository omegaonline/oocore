#include "./RegistryKey_Impl.h"

#ifdef ACE_WIN32
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

#include "./OOCore_Impl.h"

#include "./OOCore_export.h"

OOCore_Export int 
OOObject::OpenRegistryKey(const OOObject::char_t* key, OOObject::bool_t create, OOObject::RegistryKey** ppRegKey)
{
	return OOCore::Impl::RegistryKey_Impl::OpenRegistryKey(key,create,ppRegKey);
}

using namespace OOCore::Impl;

#define VALUE	"V"
#define KEY		"K"

int 
RegistryKey_Impl::OpenRegistryKey(const OOObject::char_t* key, OOObject::bool_t create, OOObject::RegistryKey** ppRegKey)
{
	bool val_type;
	ACE_CString strValue;
	if (!create && (BINDING::instance()->find(key,strValue,val_type) != 0 || val_type))
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No such key '%s'\n"),key),-1);
	}
	else if (BINDING::instance()->rebind(key,KEY,KEY) != 0)
		return -1;

	ACE_NEW_RETURN(*ppRegKey,RegistryKey_Impl(key),-1);
	(*ppRegKey)->AddRef();

	return 0;
}

RegistryKey_Impl::RegistryKey_Impl(const char* key) :
	m_strKey(key)
{
}

RegistryKey_Impl::~RegistryKey_Impl()
{
}

int
RegistryKey_Impl::QueryValue(const OOObject::char_t* name, OOObject::char_t* value, size_t* size)
{
	if (!value || !size)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL pointer!\n")),-1);
	}

	ACE_CString strKey = m_strKey + "\\" + (name ? name : "");

	bool val_type;
	ACE_CString strValue;
	if (BINDING::instance()->find(strKey.c_str(),strValue,val_type) != 0 || !val_type)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No such value '%s'\n"),name),-1);
	}

	ACE_OS::strncpy(value,strValue.c_str(),*size-1);
	if (strValue.length() >= *size)
	{
		*size = strValue.length()+1;
		return 1;
	}
	else
		return 0;
}

int 
RegistryKey_Impl::RemoveValue(const OOObject::char_t* name)
{
	ACE_CString strKey = m_strKey + "\\" + (name ? name : "");

	bool val_type;
	ACE_CString strValue;
	if (BINDING::instance()->find(strKey.c_str(),strValue,val_type) != 0 || !val_type)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No such value '%s'\n"),name),-1);
	}

	return BINDING::instance()->unbind(strKey.c_str());
}

int 
RegistryKey_Impl::SetValue(const OOObject::char_t* name, const OOObject::char_t* value)
{
	if (!value)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Invalid NULL pointer!\n")),-1);
	}

	ACE_CString strKey = m_strKey + "\\" + (name ? name : "");

	bool val_type;
	ACE_CString strValue;
	if (BINDING::instance()->find(strKey.c_str(),strValue,val_type) != 0 || !val_type)
	{
		errno = EINVAL;
		ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) No such value '%s'\n"),name),-1);
	}

	return BINDING::instance()->rebind(strKey.c_str(),value,VALUE);
}

RegistryKey_Impl::Binding::Binding() :
	m_bOpen(false)
{
	m_context.name_options()->database(ACE_TEXT("OmegaOnline.reg_db"));
	
#ifdef ACE_WIN32
#ifdef UNICODE
	m_context.name_options()->use_registry(1);
	m_context.name_options()->namespace_dir(ACE_TEXT("SOFTWARE\\OmegaOnline"));
#else
	char szBuf[MAX_PATH] = {0};
	if (::SHGetSpecialFolderPathA(NULL,szBuf,CSIDL_COMMON_APPDATA,0))
	{
		::PathAppendA(szBuf,"OmegaOnline");
		if (!::PathFileExistsA(szBuf))
		{
			if (ACE_OS::mkdir(szBuf) != 0)
				goto errored;
		}
		else if (!::PathIsDirectoryA(szBuf))
			goto trylocal;
	}
	else
	{
trylocal:
		if (::GetModuleFileNameA(g_hInstance,szBuf,MAX_PATH)!=0)
		{
			::PathRemoveFileSpecA(szBuf);
		}
	}
	
	if (szBuf[0] == 0)
	{
errored:
		ACE_ERROR((LM_ERROR,ACE_TEXT("(%P|%t) Failed to detect registry database location\n")));
		ACE_OS::abort();
	}

	m_context.name_options()->namespace_dir(szBuf);

#if (defined (ACE_HAS_WINNT4) && ACE_HAS_WINNT4 != 0)

	// Use a different base address
	m_context.name_options()->base_address((char*)(1024UL*1024*512));

	// Sometimes the base address is already in use - .NET CLR for example
	// So we check it first - I wish ACE would do this for us!
	// The problem with just defaulting to address 0x0 - which mean pick any,
	// is that ACE seems to crash creating the MEM_Map for the first time!
	MEMORY_BASIC_INFORMATION mbi;
	if (::VirtualQuery(m_context.name_options()->base_address(),&mbi,sizeof(mbi)))
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

#endif
#else
	// HUGE HACK TO GET THIS WORKING UNDER *NIX
	m_context.name_options()->namespace_dir(ACE_TEXT("/tmp/"));
#endif

	m_context.name_options()->context(ACE_Naming_Context::NODE_LOCAL);
}

RegistryKey_Impl::Binding::~Binding()
{
	m_context.close_down();
}

int 
RegistryKey_Impl::Binding::check_open()
{
	if (m_bOpen)
		return 0;

	if (m_context.open(m_context.name_options()->context(),1) != 0)
		return -1;
		
	m_bOpen = true;
	return 0;
}

int 
RegistryKey_Impl::Binding::find(const OOObject::char_t* name, ACE_CString& value, bool& val_type)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	ACE_TCHAR* pszType = 0;
	ACE_NS_WString w_val;
	int ret = m_context.resolve(name,w_val,pszType);
	if (ret==0)
		val_type = (ACE_OS::strcmp(pszType,VALUE)==0);

	ACE_OS::free(pszType);
	if (ret==0)
		value = ACE_Wide_To_Ascii(w_val.c_str()).char_rep();

	return ret;
}

int 
RegistryKey_Impl::Binding::unbind(const OOObject::char_t* name)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	return m_context.unbind(name);
}

int 
RegistryKey_Impl::Binding::rebind(const OOObject::char_t* name, const OOObject::char_t* value, const char* type)
{
	// Check we are open for business
	if (check_open() != 0)
		return -1;

	return m_context.rebind(name,value,type);
}
