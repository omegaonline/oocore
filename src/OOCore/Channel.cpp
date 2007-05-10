#include "OOCore_precomp.h"

#include "./Channel.h"
#include "./UserSession.h"

OMEGA_EXPORT_INTERFACE_DERIVED
(
	OOCore, IOutputCDR, Omega::Serialize, IFormattedStream, 
	0x21118e84, 0x2ef8, 0x4f53, 0xb4, 0xfd, 0xdb, 0xd4, 0xee, 0xc3, 0xaf, 0xc3,

	// Methods
	OMEGA_METHOD(void*,GetMessageBlock,0,())
)
const Omega::guid_t OOCore::IID_IOutputCDR = { 0x21118e84, 0x2ef8, 0x4f53, { 0xb4, 0xfd, 0xdb, 0xd4, 0xee, 0xc3, 0xaf, 0xc3 } };

using namespace Omega;
using namespace OTL;

OOCore::Channel::Channel() :
	m_pSession(0)
{
}

void OOCore::Channel::init(UserSession* pSession, ACE_CDR::UShort dest_channel_id)
{
	if (m_pSession)
		OOCORE_THROW_ERRNO(EALREADY);

	m_pSession = pSession;
	m_id = dest_channel_id;
}

Serialize::IFormattedStream* OOCore::Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr(pOuter);
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(Omega::Serialize::IID_IFormattedStream));
}

Serialize::IFormattedStream* OOCore::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pStream)
{
	// TODO We need to make the timeout cumulative - i.e. catch the first request, and use a kind
	// of 'time remaining' value to force all calls to occur within the timeout of the
	// outermost request's timeout...
	void* TODO;

	ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(15);
	
	// QI pStream for our private interface
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput;
	ptrOutput.Attach(static_cast<ObjectImpl<OutputCDR>*>(pStream->QueryInterface(OOCore::IID_IOutputCDR)));
	if (!ptrOutput)
		OOCORE_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	Serialize::IFormattedStream* pResponse = 0;
	try
	{
		if (attribs & Remoting::asynchronous)
		{
			if (!m_pSession->send_asynch(m_id,request,&deadline))
				OOCORE_THROW_LASTERROR();
		}
		else
		{
			UserSession::Request* response = 0;
			if (!m_pSession->send_synch(m_id,request,response,&deadline))
				OOCORE_THROW_LASTERROR();

			// Unpack and validate response...
			ACE_CDR::Octet ret_code = 0;
			if (!response->input()->read_octet(ret_code))
				OOCORE_THROW_LASTERROR();

			// ret_code must match the values in UserSession::process_request
			if (ret_code == 1)
			{
				OMEGA_THROW("Request timed out");
			}
			else if (ret_code == 2)
			{
				ACE_CString strDesc;
				if (!response->input()->read_string(strDesc))
					OOCORE_THROW_LASTERROR();

				ACE_CString strSrc;
				if (!response->input()->read_string(strSrc))
					OOCORE_THROW_LASTERROR();

				IException::Throw(strDesc.c_str(),strSrc.c_str());
			}
			
			// Wrap the response
			ObjectPtr<ObjectImpl<InputCDR> > ptrResponse = ObjectImpl<InputCDR>::CreateInstancePtr();
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
