

#include <stdlib.h>
#include <stdio.h>

#include "String.h"

class Tokenizer
{
public:
	Tokenizer();
	~Tokenizer();
	
	void init();
	void next_token();

private:
	// Ragel memebers
	int    m_cs;
	int*   m_stack;
	size_t m_top;
	
	size_t        m_stacksize;
	unsigned char m_char;
	size_t        m_rec;
	bool          m_skip;
		
	String m_input;
	String m_output;
		
	// These are the private members used by Ragel
	Tokenizer& operator ++ ()
	{ 
		next_char();
		return *this;
	}
		
	bool operator == (unsigned char c) const 
	{ 
		return (m_char == c); 
	}
		
	bool operator != (unsigned char c) const 
	{ 
		return (m_char != c); 
	}
		
	unsigned char operator * () const
	{ 
		return m_char; 
	}
	
	void pre_push();
		
	void do_init();
	bool do_exec();
	
	unsigned char get_char();
	unsigned char next_char_i();
	void next_char();
	void pop();
};
