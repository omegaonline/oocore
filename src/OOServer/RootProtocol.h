/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_PROTOCOL_H_INCLUDED_
#define OOSERVER_ROOT_PROTOCOL_H_INCLUDED_

#include <ace/OS.h>

namespace RootProtocol
{
	enum Operation
	{
	};

#if (defined(_MSC_VER) && _MSC_VER>=1300) 
#pragma pack(push, 1) 
#endif 

	struct Header
	{
		typedef size_t	Length;
        
		union
		{
			Length		cbSize;
			ACE_HANDLE	handle;
		};
		Operation	op;
	};

#if (defined(_MSC_VER) && _MSC_VER>=1300) 
#pragma pack(pop) 
#endif 

}

#endif // OOSERVER_ROOT_PROTOCOL_H_INCLUDED_
