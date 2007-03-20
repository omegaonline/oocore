#ifndef OOSERVER_PROTOCOL_H_INCLUDED_
#define OOSERVER_PROTOCOL_H_INCLUDED_

namespace OOServer
{
	enum RootOpCode
	{
		Open = 1,
	};
	typedef ACE_CDR::UShort RootOpCode_t;
}

#endif // OOSERVER_PROTOCOL_H_INCLUDED_
