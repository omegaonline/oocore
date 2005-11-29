#include "./Guid.h"

#include "./OOCore.h"

const OOObject::guid_t OOObject::guid_t::NIL = {0};

namespace OOCore
{
	namespace Impl
	{
		static OOObject::guid_t init_guid(ACE_Utils::UUID& uuid);
	};
};

static OOObject::guid_t 
OOCore::Impl::init_guid(ACE_Utils::UUID& uuid)
{
	OOObject::guid_t ret;

	ret.Data1 = uuid.timeLow();
	ret.Data2 = uuid.timeMid();
	ret.Data3 = uuid.timeHiAndVersion();
	ret.Data4[0] = uuid.clockSeqHiAndReserved();
	ret.Data4[1] = uuid.clockSeqLow();
	ret.Data4[2] = uuid.node()->nodeID()[0];
	ret.Data4[3] = uuid.node()->nodeID()[1];
	ret.Data4[4] = uuid.node()->nodeID()[2];
	ret.Data4[5] = uuid.node()->nodeID()[3];
	ret.Data4[6] = uuid.node()->nodeID()[4];
	ret.Data4[7] = uuid.node()->nodeID()[5];

	return ret;
}

OOObject::guid_t 
OOCore::Impl::create_guid(const ACE_Utils::UUID& uuid)
{
	return init_guid(const_cast<ACE_Utils::UUID&>(uuid));
}

OOObject::guid_t 
OOCore::Impl::create_guid(const ACE_CString& uuidString)
{
	return init_guid(ACE_Utils::UUID(uuidString));
}

bool 
OOObject::guid_t::operator ==(const OOObject::guid_t& rhs) const
{
	return (Data1==rhs.Data1 &&
			Data2==rhs.Data2 &&
			Data3==rhs.Data3 &&
			ACE_OS::memcmp(Data4,rhs.Data4,8)==0);
}

bool 
OOObject::guid_t::operator <(const OOObject::guid_t& rhs) const
{
	if (Data1>rhs.Data1)
		return false;

	if (Data2>rhs.Data2)
		return false;
	
	if (Data3>rhs.Data3)
		return false;

	return ACE_OS::memcmp(Data4,rhs.Data4,8)<0;
}

ACE_TString 
OOCore::Impl::guid_to_string(const OOObject::guid_t& guid)
{
	ACE_TCHAR buf[37];
	ACE_OS::sprintf(buf,
        ACE_TEXT("%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"),
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
		guid.Data4[2],
		guid.Data4[3],
		guid.Data4[4],
		guid.Data4[5],
		guid.Data4[6],
		guid.Data4[7]);

	return ACE_TString(buf);
}
