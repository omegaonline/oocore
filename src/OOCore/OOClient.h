#ifndef _OOCORE_OOCLIENT_H_INCLUDED_
#define _OOCORE_OOCLIENT_H_INCLUDED_

#include "./Object.h"

#include "./OOCore_export.h"

int OOCore_Export OOClient_Init();
void OOCore_Export OOClient_Term();
int OOCore_Export OOClient_CreateObject(const OOObj::char_t* service_name, const OOObj::GUID& iid, OOObj::Object** ppVal);

// Useful little helper (T must be derived from OOObj::Object)
template <class T>
int OOClient_CreateObject(const OOObj::char_t* service_name, T** ppVal)
{
	return OOClient_CreateObject(service_name,T::IID,reinterpret_cast<OOObj::Object**>(ppVal));
}

#endif // _OOCORE_OOCLIENT_H_INCLUDED_
