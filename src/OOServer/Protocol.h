#ifndef OOSERVER_PROTOCOL_H_INCLUDED_
#define OOSERVER_PROTOCOL_H_INCLUDED_

namespace OOServer
{
	enum RootOpCode
	{
<<<<<<< .mine
		KeyExists = 1,
		CreateKey,
		DeleteKey,
		EnumSubKeys,
		ValueType,
		GetStringValue,
		GetUInt32Value,
		SetStringValue,
		SetUInt32Value,
		EnumValues,
		DeleteValue,

=======
		Open = 1,
>>>>>>> .r249
	};
	typedef ACE_CDR::UShort RootOpCode_t;
}

#endif // OOSERVER_PROTOCOL_H_INCLUDED_
