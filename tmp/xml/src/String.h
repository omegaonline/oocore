
#include <stdlib.h>

class String
{
public:
	String();
	String(const String& rhs);
	String& operator = (const String& rhs);
	~String();
	
	bool empty() const;
	unsigned char pop();
	void push(unsigned char c);
	void clear();
	
	void dump(int offset = 0);
	
private:
	class StringNode
	{
	public:
		StringNode();
	
		void addref();
		void release();
		
		bool empty() const;
		unsigned char pop();
		void push(unsigned char c);
		
		void dump(int offset);
		
	private:
		~StringNode();
		
		size_t         m_refcount;
		unsigned char* m_data;
		size_t         m_size;
		unsigned char* m_ptr;
		
		void grow();
	};
	StringNode* m_node;
};
