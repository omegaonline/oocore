//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_OBJECT_MARSHALLER_H_INCLUDED_
#define OOCORE_OBJECT_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"

namespace Marshall_A
{

class Object_Marshaller
{
public:
	Object_Marshaller(const OOObj::guid_t& iid, OOObj::Object** ppObj, bool in = false) :
	  m_iid(iid), m_node(0), m_objref(ppObj), m_in(in), m_out(true)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = (in ? *ppObj : 0);
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
	}

	Object_Marshaller(const OOObj::guid_t& iid, OOObj::Object* pObj) :
		m_iid(iid), m_node(0), m_in(true), m_out(false)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = pObj;
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
		m_objref = &m_node->m_obj;
	}

	Object_Marshaller() :
		m_iid(OOObj::guid_t::NIL), m_node(0), m_in(false), m_out(false)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = NULL;
		m_node->m_refcount = 1;
		m_objref = &m_node->m_obj;
	}

	template <class TYPE>
	Object_Marshaller(TYPE** ppObj, bool in = false) :
	m_iid(TYPE::IID), m_node(0), m_objref(reinterpret_cast<OOObj::Object**>(ppObj)), m_in(in), m_out(true)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = (in ? *ppObj : 0);
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
	}

	template <class TYPE>
	Object_Marshaller(TYPE* pObj) :
		m_iid(TYPE::IID), m_node(0), m_in(true), m_out(false)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = pObj;
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
		m_objref = &m_node->m_obj;
	}

	Object_Marshaller(const Object_Marshaller& rhs) :
		m_iid(rhs.m_iid), m_node(rhs.m_node), m_objref(rhs.m_objref), m_in(rhs.m_in), m_out(rhs.m_out)
	{
		if (m_node)
			++m_node->m_refcount;
	}

	virtual ~Object_Marshaller()
	{
		if (m_node && --m_node->m_refcount == 0)
		{
			if (m_node->m_obj)
				m_node->m_obj->Release();
			delete m_node;
		}
	}

	ACE_CDR::Boolean read(Marshaller_Base& mshl, OOCore::InputStream* input, bool response);
	ACE_CDR::Boolean write(Marshaller_Base& mshl, OOCore::OutputStream* output, bool response) const;

	OOObj::Object** objref()
	{
		return m_objref;
	}

	bool responds() const
	{
		return m_out;
	}

private:
	struct data_node
	{
		OOObj::Object* m_obj;
		unsigned long m_refcount;
	};
	
	OOObj::guid_t m_iid;
	data_node* m_node;
	OOObj::Object** m_objref;
	bool m_in;
	bool m_out;
};

namespace IOWrappers
{
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, Object_Marshaller& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const Object_Marshaller& val, bool response);
	bool arg_responds(const Object_Marshaller& val);
};

};

#endif // OOCORE_OBJECT_MARSHALLER_H_INCLUDED_
