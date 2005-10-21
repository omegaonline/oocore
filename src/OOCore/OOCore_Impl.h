//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#pragma once

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

template<>
struct std::less<ACE_Active_Map_Manager_Key>
{
	bool operator () (const ACE_Active_Map_Manager_Key &lhs, const ACE_Active_Map_Manager_Key &rhs) const
	{
		return (lhs.slot_index() < rhs.slot_index() ||
				lhs.slot_generation() < rhs.slot_generation());
	}
};

ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_Active_Map_Manager_Key& key);
ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_Active_Map_Manager_Key& key);
ACE_CDR::Boolean operator >>(ACE_InputCDR& input, ACE_CDR::Boolean& val);
ACE_CDR::Boolean operator <<(ACE_OutputCDR& output, const ACE_CDR::Boolean& val);

typedef bool (*CONDITION_FN)(void*);
int OOCore_RunReactorEx(ACE_Time_Value* timeout = 0, CONDITION_FN cond_fn = 0, void* p = 0);
int OOCore_PostRequest(ACE_Method_Request* req);