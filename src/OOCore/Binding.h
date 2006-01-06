//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_BINDING_H_INCLUDED_
#define OOCORE_BINDING_H_INCLUDED_

#include <ace/Naming_Context.h>
#include <ace/Singleton.h>
#include <ace/Thread_Mutex.h>

#include "./OOCore_export.h"

namespace OOCore
{
namespace Impl
{

class Binding
{
public:
	int launch(bool bAsServer = false);
	
	int find(const ACE_TCHAR* name, ACE_TString& value);
	int rebind(const ACE_TCHAR* name, const ACE_TCHAR* value);
	int unbind(const ACE_TCHAR* name);

private:
	friend class ACE_Singleton<Binding, ACE_Thread_Mutex>;

	Binding();
	virtual ~Binding();

	bool m_unbind_pid;
	bool m_bOpen;
	ACE_Naming_Context m_context;
	
	int launch_server();
	int check_open();
};

typedef ACE_Singleton<Binding, ACE_Thread_Mutex> BINDING;

};
};

#endif // _OOCORE_BINDING_H_INCLUDED_
