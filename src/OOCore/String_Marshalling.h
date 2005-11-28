//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_STRING_MARSHALLING_H_INCLUDED_
#define OOCORE_STRING_MARSHALLING_H_INCLUDED_

namespace Marshall_A
{

class StringHolder
{
public:
	StringHolder(OOObj::char_t* str) :
		m_str(str)
	{
		ACE_NEW_NORETURN(m_node,node);
		m_node->m_str = str;
		m_node->m_refcount = 1;
	}

	StringHolder(const StringHolder& rhs) 
	{
		m_node = rhs.m_node;
		m_node->m_refcount++;
	}

	virtual ~StringHolder()
	{
		if (--m_node->m_refcount==0)
		{
			delete [] m_str;
			delete m_node;
		}
	}

	const OOObj::char_t*& const_ref() const
	{
		return m_node->m_str;
	}

private:
	StringHolder() {};

	struct node
	{
		OOObj::char_t* m_str;
		unsigned long m_refcount;
	};

	node* m_node;
	OOObj::char_t* m_str;
};

namespace IOWrappers
{
	bool read_param(Marshaller_Base* mshl, OOCore::InputStream* input, StringHolder& val, bool response);
	bool write_param(Marshaller_Base* mshl, OOCore::OutputStream* output, const StringHolder& val, bool response);
};

};

#endif // OOCORE_STRING_MARSHALLING_H_INCLUDED_
