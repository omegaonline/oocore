//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef _OOCORE_OOCORE_IMPL_H_INCLUDED_
#define _OOCORE_OOCORE_IMPL_H_INCLUDED_

#include <ace/Active_Map_Manager.h>
#include <ace/Functor_String.h>
#include <ace/CDR_Stream.h>
#include <ace/Activation_Queue.h>

template<>
class ACE_Less_Than<ACE_Active_Map_Manager_Key>
{
public:
	int operator () (const ACE_Active_Map_Manager_Key &lhs, const ACE_Active_Map_Manager_Key &rhs) const
	{
		return (lhs.slot_index() < rhs.slot_index() ||
				lhs.slot_generation() < rhs.slot_generation());
	}
};

#include <functional>

namespace std
{
template<>
struct less<ACE_Active_Map_Manager_Key>
{
	bool operator () (const ACE_Active_Map_Manager_Key &lhs, const ACE_Active_Map_Manager_Key &rhs) const
	{
		return (lhs.slot_index() < rhs.slot_index() ||
				lhs.slot_generation() < rhs.slot_generation());
	}
};
};

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_Active_Map_Manager_Key& key);
ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_Active_Map_Manager_Key& key);
ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_CDR::Boolean& val);
ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_CDR::Boolean& val);

// This is a shoddy fixup for compilers with broken explicit template specialisation
#if (__GNUC__) && (__GNUC__ <= 3)
	#define EXPLICIT_TEMPLATE(m,t)	template m<t>
#else
	#define EXPLICIT_TEMPLATE(m,t)	m<t>
#endif

#endif // _OOCORE_OOCORE_IMPL_H_INCLUDED_
