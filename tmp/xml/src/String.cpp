
#include <stdio.h>
#include "String.h"
#include <new>

String::String() : m_node(NULL)
{}

String::String(const String& rhs) : m_node(rhs.m_node)
{
	if (m_node)
		m_node->addref();
}

String& String::operator = (const String& rhs)
{
	if (this != &rhs)
	{
		if (m_node)
			m_node->release();
			
		m_node = rhs.m_node;
		
		if (m_node)
			m_node->addref();
	}
	return *this;
}

String::~String()
{
	if (m_node)
		m_node->release();
}

bool String::empty() const
{
	return (!m_node || m_node->empty());
}
	
unsigned char String::pop()
{
	if (!m_node)
		return '\0';
	
	return m_node->pop();
}
	
void String::push(unsigned char c)
{
	if (!m_node)
		m_node = new StringNode();
		
	m_node->push(c);
}

void String::clear()
{
	if (m_node)
		m_node->release();
		
	m_node = NULL;
}

void String::dump(int offset)
{
	if (m_node)
		m_node->dump(offset);
}

String::StringNode::StringNode() :
		m_refcount(1),
		m_data(NULL),
		m_size(0),
		m_ptr(NULL)
{ }

String::StringNode::~StringNode()
{
	free(m_data);	
}

void String::StringNode::addref()
{
	++m_refcount;
}

void String::StringNode::release()
{
	if (--m_refcount == 0)
		delete this;
}

void String::StringNode::grow()
{
	if (static_cast<size_t>(m_ptr-m_data) >= m_size)
	{
		size_t new_size = (m_size == 0 ? 256 : m_size*2);
		unsigned char* new_data = static_cast<unsigned char*>(realloc(m_data,new_size));
		if (!new_data)
			throw std::bad_alloc();
			
		m_ptr = new_data + (m_ptr-m_data);
		m_data = new_data;
		m_size = new_size;
	}
}	

bool String::StringNode::empty() const
{
	return (m_ptr == m_data);
}

unsigned char String::StringNode::pop()
{
	if (m_ptr == m_data)
		return '\0';
	
	return *(--m_ptr);
}

void String::StringNode::push(unsigned char c)
{
	grow();
	
	*(m_ptr++) = c;
}

void String::StringNode::dump(int offset)
{
	if (m_ptr != m_data)
		printf("%.*s",(int)(m_ptr-m_data)+offset,m_data);
}
