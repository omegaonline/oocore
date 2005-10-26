#include "./Guid.h"

OOObj::GUID::GUID(const ACE_Utils::UUID& uuid)
{
	init_i(const_cast<ACE_Utils::UUID&>(uuid));
}

OOObj::GUID::GUID()
{
	init_i(ACE_Utils::UUID::NIL_UUID);
}

OOObj::GUID::GUID(const ACE_CString& uuidString)
{
	ACE_Utils::UUID uuid(uuidString);
	init_i(uuid);
}

void OOObj::GUID::init_i(ACE_Utils::UUID& uuid)
{
	Data1 = uuid.timeLow();
	Data2 = uuid.timeMid();
	Data3 = uuid.timeHiAndVersion();
	Data4[0] = uuid.clockSeqHiAndReserved();
	Data4[1] = uuid.clockSeqLow();
	Data4[2] = uuid.node()->nodeID()[0];
	Data4[3] = uuid.node()->nodeID()[1];
	Data4[4] = uuid.node()->nodeID()[2];
	Data4[5] = uuid.node()->nodeID()[3];
	Data4[6] = uuid.node()->nodeID()[4];
	Data4[7] = uuid.node()->nodeID()[5];
}

bool OOObj::GUID::operator ==(const GUID& rhs) const
{
	return (Data1==rhs.Data1 &&
			Data2==rhs.Data2 &&
			Data3==rhs.Data3 &&
			ACE_OS::memcmp(Data4,rhs.Data4,8)==0);
}

bool OOObj::GUID::operator !=(const GUID& rhs) const
{
	return !(*this==rhs);
}

bool OOObj::GUID::operator <(const GUID& rhs) const
{
	if (Data1>rhs.Data1)
		return false;

	if (Data2>rhs.Data2)
		return false;
	
	if (Data3>rhs.Data3)
		return false;

	return ACE_OS::memcmp(Data4,rhs.Data4,8)<0;
}

ACE_TString OOObj::GUID::to_string() const
{
	ACE_TCHAR buf[37];
	ACE_OS::sprintf(buf,
        ACE_TEXT("%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x"),
        this->Data1,
        this->Data2,
        this->Data3,
        this->Data4[0],
        this->Data4[1],
		this->Data4[2],
		this->Data4[3],
		this->Data4[4],
		this->Data4[5],
		this->Data4[6],
		this->Data4[7]);

	return ACE_TString(buf);
}

ACE_CDR::Boolean OOCore_Export OOObj::operator >>(ACE_InputCDR& input, OOObj::GUID& guid)
{
	input.read_ulong(guid.Data1);
	input.read_ushort(guid.Data2);
	input.read_ushort(guid.Data3);
	input.read_octet_array(guid.Data4,8);

	return input.good_bit();
}

ACE_CDR::Boolean OOCore_Export OOObj::operator <<(ACE_OutputCDR& output, const OOObj::GUID& guid)
{
	output << guid.Data1;
	output << guid.Data2;
	output << guid.Data3;
	output.write_octet_array(guid.Data4,8);

	return output.good_bit();
}
