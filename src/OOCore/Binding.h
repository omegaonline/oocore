#pragma once

#include <ace/Naming_Context.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./OOCore_export.h"

// This is a class conatining statics used as a wrapper for exporting
class OOCore_Export OOCore_Binding
{
public:
	OOCore_Binding();
	virtual ~OOCore_Binding();

	// Returns:
	// 0 (not running),
	// 1 (other process running)
	// -1 (error)
	int launch(bool bAsServer = false);
	
	int find(const ACE_TCHAR* name, ACE_NS_WString& value);
	int rebind(const ACE_TCHAR* name, const ACE_NS_WString& value);

	const ACE_TCHAR* dll_name(void);
	const ACE_TCHAR* name(void);
	
private:
	bool m_unbind_pid;
	ACE_Naming_Context m_context;
	
	int launch_server();
};

typedef ACE_DLL_Singleton_T<OOCore_Binding, ACE_Thread_Mutex> BINDING;
OOCORE_SINGLETON_DECLARE(ACE_DLL_Singleton_T,OOCore_Binding,ACE_Thread_Mutex);