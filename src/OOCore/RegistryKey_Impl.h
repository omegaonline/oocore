#ifndef OOCORE_REGISTRYKEY_IMPL_H_INCLUDED_
#define OOCORE_REGISTRYKEY_IMPL_H_INCLUDED_

#include <ace/Naming_Context.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./OOUtil.h"

namespace OOCore
{
namespace Impl
{

class RegistryKey_Impl : 
	public OOUtil::Object_Impl<OOObject::RegistryKey>
{
public:
	static int OpenRegistryKey(const OOObject::char_t* key, OOObject::bool_t create, OOObject::RegistryKey** ppRegKey);

private:
	class Binding
	{
		friend class ACE_DLL_Singleton<Binding, ACE_Thread_Mutex>;

	public:
		int find(const OOObject::char_t* name, ACE_CString& value, bool& val_type);
		int unbind(const OOObject::char_t* name);
		int rebind(const OOObject::char_t* name, const OOObject::char_t* value, const char* type);
				
	private:
		Binding();
		virtual ~Binding();

		bool m_bOpen;
		ACE_Naming_Context m_context;

		int check_open();
		
	};
	typedef ACE_DLL_Singleton<Binding, ACE_Thread_Mutex> BINDING;

	RegistryKey_Impl(const char* key);
	virtual ~RegistryKey_Impl();

	ACE_CString m_strKey;

// OOObject::RegistryKey members
public:
	int QueryValue(const OOObject::char_t* name, OOObject::char_t* value, size_t* size);
	int RemoveValue(const OOObject::char_t* name);
	int SetValue(const OOObject::char_t* name, const OOObject::char_t* value);
};

};
};

#endif // OOCORE_REGISTRYKEY_IMPL_H_INCLUDED_
