//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_ARRAY_MARSHALLER_H_INCLUDED_
#define _OOCORE_ARRAY_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"
#include "./OOCore.h"
#include "./OOCore_Impl.h"

template <class TYPE>
class OOCore_Array_Marshaller
{
public:
	OOCore_Array_Marshaller(OOObj::uint32_t index, TYPE** pArr, bool in = false) :
		m_index(index), m_in(in), m_out(true), m_node(0), m_dataref(pArr)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_data = 0;
		m_node->m_refcount = 1;
		m_node->m_del = false;
	}

	OOCore_Array_Marshaller(OOObj::uint32_t index, TYPE* pArr = 0) :
		m_index(index), m_in(true), m_out(false), m_node(0)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_data = pArr;
		m_node->m_refcount = 1;
		m_node->m_del = false;
		m_dataref = &m_node->m_data;
	}

	OOCore_Array_Marshaller(const OOCore_Array_Marshaller& rhs) :
		m_index(rhs.m_index), m_in(rhs.m_in), m_out(rhs.m_out), m_node(rhs.m_node), m_dataref(rhs.m_dataref)
	{
		if (m_node)
			++m_node->m_refcount;
	}

	virtual ~OOCore_Array_Marshaller()
	{
		if (m_node && --m_node->m_refcount == 0)
		{
			if (m_node->m_del)
				OOCore_Free(m_node->m_data);

			delete m_node;
		}
	}

	bool write(OOCore_Marshaller_Base& mshl, ACE_OutputCDR& output, bool response) const
	{
		if (m_node==0)
			return false;

		if (!response)
		{
			if (!output.write_ulong(m_index))
				return false;

			if (!output.write_boolean(m_in))
				return false;

			if (!output.write_boolean(m_out))
				return false;
		}

		if ((!response && m_in) || (response && m_out))
		{
			OOObj::uint32_t count = 0;
			if (m_dataref == &m_node->m_data)
				count = mshl.EXPLICIT_TEMPLATE(param,OOObj::uint32_t)(m_index)->value();
			else
				count = *mshl.EXPLICIT_TEMPLATE(param,OOObj::uint32_t*)(m_index)->value();

			for (OOObj::uint32_t i=0;i<count;++i)
				if (!write_param(&mshl,output,(*m_dataref)[i],response))
					return false;	
		}
				
		return true;
	}

	bool read(OOCore_Marshaller_Base& mshl, ACE_InputCDR& input, bool response)
	{
		if (m_node==0)
			return false;

		if (!response)
		{
			// m_index is read in OOCore_Stub_Marshaller::unpack_i()

			if (!input.read_boolean(m_in))
				return false;

			if (!input.read_boolean(m_out))
				return false;
		}

		// Dereference the array count
		if ((!response && m_in) || (response && m_out))
		{
			OOObj::uint32_t count;
			
			if (!response)
				count = mshl.EXPLICIT_TEMPLATE(param,OOObj::uint32_t)(m_index)->value();
			else
				count = *mshl.EXPLICIT_TEMPLATE(param,OOObj::uint32_t*)(m_index)->value();

			if (count>0 && m_node->m_data == 0)
			{
				ACE_NEW_MALLOC_ARRAY_RETURN(m_node->m_data,static_cast<TYPE*>(OOCore_Alloc(count * sizeof(TYPE))),TYPE,count,false);

				m_node->m_del = true;
			}

			for (OOObj::uint32_t i=0;i<count;++i)
				if (!read_param(&mshl,input,m_node->m_data[i],response))
					return false;
						
			if (response)
			{
				*m_dataref = m_node->m_data;
				m_node->m_del = false;
			}
		}
		else if (!response)
		{
			m_node->m_del = true;
		}
			 
		return true;
	}

	TYPE* data()
	{
		return m_node->m_data;
	}

	TYPE** dataref()
	{
		return m_dataref;
	}

	bool responds() const
	{
		return m_out;
	}
	
private:
	struct data_node
	{
		TYPE* m_data;
		unsigned long m_refcount;
		bool m_del;
	};

	const OOObj::uint32_t m_index;
	bool m_in;
	bool m_out;
	data_node* m_node;
	TYPE** m_dataref;
};

template <class T>
bool read_param(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input, OOCore_Array_Marshaller<T>& val, bool response)
{
	return val.read(*mshl,input,response);
}

template <class T>
bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, const OOCore_Array_Marshaller<T>& val, bool response)
{
	return val.write(*mshl,output,response);
}

template <class T>
bool arg_responds(const OOCore_Array_Marshaller<T>& val)
{
	return val.responds();
}

#endif // _OOCORE_ARRAY_MARSHALLER_H_INCLUDED_
