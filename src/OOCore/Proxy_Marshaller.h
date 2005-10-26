//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_PROXY_MARSHALLER_H_INCLUDED_
#define _OOCORE_PROXY_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_Proxy_Marshaller : public OOCore_Marshaller_Base
{
	friend class OOCore_ProxyStub_Handler;

public:
	template <class T>
	OOCore_Proxy_Marshaller& operator <<(const T& val)
	{
        if (!m_failed)
			m_failed = !write_param(this,m_output,val,false);
		
		if (!m_failed)
			m_failed = (pack_param(val,arg_responds(val)) == 0);

		return *this;
	}

	template <class T>
	OOCore_Proxy_Marshaller& operator <<(T* val)
	{
		if (val == 0)
			m_failed = true;

		if (!m_failed)
			m_failed = !m_output.write_ulong(-1);
		
		if (!m_failed)
			m_failed = !write_param(this,m_output,val,false);

		if (!m_failed)
			m_failed = (pack_param(val,true) == 0);

		return *this;
	}

	// Specials
	OOCore_Proxy_Marshaller& operator <<(const ACE_CDR::Char* val);
	
	// Do the work
	int operator ()(ACE_Time_Value& wait);
	int operator ()(ACE_Time_Value* wait = 0);

	ACE_OutputCDR& output()
	{
		return m_output;
	}

private:
	OOCore_Proxy_Marshaller();
	OOCore_Proxy_Marshaller(OOCore_ProxyStub_Handler* handler, bool sync);

	ACE_OutputCDR m_output;
	ACE_Active_Map_Manager_Key m_trans_key;

	int send_and_recv(ACE_Time_Value* wait);
};

#endif // _OOCORE_PROXY_MARSHALLER_H_INCLUDED_
