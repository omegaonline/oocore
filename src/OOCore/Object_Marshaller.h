//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_OBJECT_MARSHALLER_H_INCLUDED_
#define _OOCORE_OBJECT_MARSHALLER_H_INCLUDED_

#include "./Marshaller.h"
#include "./OOCore_Impl.h"
#include "./Object_Impl.h"

#include "./OOCore_export.h"

class OOCore_Export OOCore_Object_Marshaller
{
public:
	OOCore_Object_Marshaller(const OOObj::GUID& iid, OOObj::Object** ppObj, bool in = false) :
	  m_iid(iid), m_node(0), m_objref(ppObj), m_in(in), m_out(true)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = (in ? *ppObj : 0);
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
	}

	OOCore_Object_Marshaller(const OOObj::GUID& iid = OOObj::GUID::GUID_NIL, OOObj::Object* pObj = NULL, bool in = false) :
		m_iid(iid), m_node(0), m_in(in), m_out(false)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = pObj;
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
		m_objref = &m_node->m_obj;
	}

	template <class TYPE>
	OOCore_Object_Marshaller(TYPE** ppObj, bool in = false) :
	m_iid(TYPE::IID), m_node(0), m_objref(reinterpret_cast<OOObj::Object**>(ppObj)), m_in(in), m_out(true)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = (in ? *ppObj : 0);
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
	}

	template <class TYPE>
	OOCore_Object_Marshaller(TYPE* pObj) :
		m_iid(TYPE::IID), m_node(0), m_in(true), m_out(false)
	{
		ACE_NEW(m_node,data_node);
		m_node->m_obj = pObj;
		if (m_node->m_obj)
			m_node->m_obj->AddRef();
		m_node->m_refcount = 1;
		m_objref = &m_node->m_obj;
	}

	OOCore_Object_Marshaller(const OOCore_Object_Marshaller& rhs) :
		m_iid(rhs.m_iid), m_node(rhs.m_node), m_objref(rhs.m_objref), m_in(rhs.m_in), m_out(rhs.m_out)
	{
		if (m_node)
			++m_node->m_refcount;
	}

	virtual ~OOCore_Object_Marshaller()
	{
		if (m_node && --m_node->m_refcount == 0)
		{
			if (m_node->m_obj)
				m_node->m_obj->Release();
			delete m_node;
		}
	}

	ACE_CDR::Boolean read(OOCore_Marshaller_Base& mshl, ACE_InputCDR& input, bool response);
	ACE_CDR::Boolean write(OOCore_Marshaller_Base& mshl, ACE_OutputCDR& output, bool response) const;

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
	
	OOObj::GUID m_iid;
	data_node* m_node;
	OOObj::Object** m_objref;
	bool m_in;
	bool m_out;
};

bool read_param(OOCore_Marshaller_Base* mshl, ACE_InputCDR& input, OOCore_Object_Marshaller& val, bool response);
bool write_param(OOCore_Marshaller_Base* mshl, ACE_OutputCDR& output, const OOCore_Object_Marshaller& val, bool response);
bool arg_responds(const OOCore_Object_Marshaller& val);

#endif // _OOCORE_OBJECT_MARSHALLER_H_INCLUDED_
