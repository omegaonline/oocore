#include "OOCore_precomp.h"

#include "./Channel.h"
#include "./UserSession.h"

const Omega::guid_t OOCore::IID_InputCDR = { 0xe39658, 0x1774, 0x4f02, { 0x86, 0xe5, 0xfc, 0xe8, 0xbe, 0x3c, 0xe4, 0xa5 } };
OMEGA_DECLARE_IID_TRAITS(OOCore,InputCDR)

const Omega::guid_t OOCore::IID_OutputCDR = { 0x21118e84, 0x2ef8, 0x4f53, { 0xb4, 0xfd, 0xdb, 0xd4, 0xee, 0xc3, 0xaf, 0xc3 } };
OMEGA_DECLARE_IID_TRAITS(OOCore,OutputCDR)

using namespace Omega;
using namespace OTL;

Channel::Channel() :
	m_pSession(0)
{
}

void Channel::init(UserSession* pSession, ACE_CDR::UShort dest_channel_id)
{
	if (m_pSession)
		OOCORE_THROW_ERRNO(EALREADY);

	m_pSession = pSession;
	m_id = dest_channel_id;
}

Serialize::IFormattedStream* Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OOCore::OutputCDR
	ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrOutput = ObjectImpl<OOCore::OutputCDR>::CreateObjectPtr(pOuter);
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(Omega::Serialize::IID_IFormattedStream));
}

Serialize::IFormattedStream* Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pStream)
{
	// We need to make the timeout cumulative - i.e. catch the first request, and use a kind
	// of 'time remaining' value to force all calls to occur within the timeout of the
	// outermost requests timeout...
	void* TODO;

	ACE_Time_Value deadline(ACE_OS::gettimeofday());
	deadline += 5;

	// QI pStream for our private interface
	ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrOutput;
	ptrOutput.Attach(static_cast<ObjectImpl<OOCore::OutputCDR>*>(pStream->QueryInterface(OOCore::IID_OutputCDR)));
	if (!ptrOutput)
		OOCORE_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = ptrOutput->GetMessageBlock();

	Serialize::IFormattedStream* pResponse = 0;
	try
	{
		if (attribs & Remoting::asynchronous)
		{
			if (m_pSession->send_asynch(m_id,request,&deadline) != 0)
				OOCORE_THROW_LASTERROR();
		}
		else
		{
			UserSession::Request* response = 0;
			if (m_pSession->send_synch(m_id,request,response,&deadline) != 0)
				OOCORE_THROW_LASTERROR();

			// Unpack and validate response...
			ACE_CDR::ULong ret_code = 0;
			if (!response->input()->read_ulong(ret_code))
				OOCORE_THROW_LASTERROR();

			// ret_code must match the values in UserSession::process_request
			switch (ret_code)
			{
			case 1: OMEGA_THROW("Remote service failed to resolve ObjectManager"); break;
			case 2: OMEGA_THROW("Request timed out"); break;
			case 3: OMEGA_THROW("Remote service failed to wrap request"); break;
			case 4: OMEGA_THROW("Remote service failed to create response"); break;
			case 5: OMEGA_THROW("Remote service failed to marshal exception"); break;
			case 6: OMEGA_THROW("Remote service failed to send response"); break;
			case 0:
			default:
				break;
			}

			// Wrap the response
			ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrResponse = ObjectImpl<OOCore::InputCDR>::CreateObjectPtr();
			ptrResponse->init(*response->input());
			pResponse = ptrResponse.Detach();
		}
	}
	catch (...)
	{
		request->release();
		throw;
	}

	request->release();
	return pResponse;
}
