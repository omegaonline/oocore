#ifndef OOSERVER_PROTOCOL_H_INCLUDED_
#define OOSERVER_PROTOCOL_H_INCLUDED_

namespace Root
{
	enum RootOpCode
	{
		// Root -> User
		End,

		// User -> Root
		KeyExists,
		CreateKey,
		DeleteKey,
		EnumSubKeys,
		ValueType,
		GetStringValue,
		GetUInt32Value,
		GetBinaryValue,
		SetStringValue,
		SetUInt32Value,
		SetBinaryValue,
		EnumValues,
		DeleteValue,
	};
	typedef ACE_CDR::UShort RootOpCode_t;
}

#endif // OOSERVER_PROTOCOL_H_INCLUDED_
