//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_PROXY_MARSHALLER_H_INCLUDED_
#define OOCORE_PROXY_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"
#include "./Array_Marshaller.h"

namespace Impl
{

class Proxy_Marshaller : public Marshaller_Base
{
public:
	Proxy_Marshaller();
	Proxy_Marshaller(OOCore::ProxyStubManager* manager, OOObj::bool_t sync, OOCore::OutputStream* output, OOObj::uint32_t trans_id);

	template <class T>
	Proxy_Marshaller& operator <<(const T& val)
	{
        if (!m_failed)
			m_failed = !IOWrappers::write_param(this,m_output,val,false);
		
		if (!m_failed)
			m_failed = (pack_param(val,IOWrappers::arg_responds(val)) == 0);

		return *this;
	}

	template <class T>
	Proxy_Marshaller& operator <<(T* val)
	{
		if (val == 0)
			m_failed = true;

		if (!m_failed)
			m_failed = (m_output->WriteULong(-1)==0 ? false : true);
		
		if (!m_failed)
			m_failed = !IOWrappers::write_param(this,m_output,val,false);

		if (!m_failed)
			m_failed = (pack_param(val,true) == 0);

		return *this;
	}
	
	// Do the work
	int operator ()(ACE_Time_Value& wait);
	int operator ()(ACE_Time_Value* wait = 0);

	/*ACE_OutputCDR& output()
	{
		return m_output;
	}*/

private:
	OOCore::Object_Ptr<OOCore::OutputStream> m_output;
	OOObj::uint32_t m_trans_id;
	OOObj::bool_t m_sync;
};

};

#endif // OOCORE_PROXY_MARSHALLER_H_INCLUDED_
