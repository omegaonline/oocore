
#include <stdio.h>
#include <assert.h>

#include "Tokenizer.h"

Tokenizer::Tokenizer() : 
		m_stack(NULL), 
		m_stacksize(0), 
		m_char('\0')
{
}

Tokenizer::~Tokenizer()
{
	free(m_stack);
}

void Tokenizer::init()
{
	do_init();
	
	next_char();
}

void Tokenizer::pre_push()
{
	if (!m_stack)
	{
		m_stacksize = 256;
		m_stack = static_cast<int*>(malloc(m_stacksize*sizeof(int)));
	}
	else if (m_top == m_stacksize-1)
	{
		int* new_stack = static_cast<int*>(realloc(m_stack,m_stacksize*2*sizeof(int)));
		m_stack = new_stack;
		m_stacksize *= 2;
	}
}

unsigned char Tokenizer::next_char_i()
{
	unsigned char c = '\0';
	
	if (m_input.empty())
		c = get_char();
	else
		c = m_input.pop();
		
	return c;
}

void Tokenizer::next_char()
{
	unsigned char c = next_char_i();
	if (c == '\r')
	{
		c = '\n';
		
		unsigned char n = next_char_i();
		if (n != '\n')
			m_input.push(n);
	}
	
	m_char = c;
}

void Tokenizer::subst_entity()
{
	printf("Entity!\n");
	
	m_entity.clear();
}

void Tokenizer::subst_char()
{
	//printf("CHARREF: '");
	//m_entity.dump();
	//printf("'\n");
	
	m_entity.clear();
	m_output.push('1');
}

void Tokenizer::subst_hex()
{
	printf("CHARREF: '0x");
	m_entity.dump();
	printf("'\n");
	
	m_entity.clear();
}

void Tokenizer::token(const char* s, int offset)
{
	printf("%s: '",s);
	m_output.dump(offset);
	printf("'\n");
	
	m_output.clear();
}

void Tokenizer::next_token()
{
	do_exec();
	
	printf("State: %u\n",m_cs);
	
	
}
