#include "OOServer.h"

#include "./Channel.h"
#include "./UserManager.h"

const Omega::guid_t OOServer::IID_InputCDR = { 0x77b39017, 0xeca2, 0x4073, { 0xa6, 0x65, 0x1c, 0x3c, 0xa7, 0x54, 0x20, 0x62 } };
OMEGA_DECLARE_IID_TRAITS(OOServer,InputCDR)

const Omega::guid_t OOServer::IID_OutputCDR = { 0x5e8c6ed6, 0xe8b7, 0x4bc6, { 0xa2, 0x65, 0x79, 0xe3, 0xb5, 0x51, 0xa4, 0x3e } };
OMEGA_DECLARE_IID_TRAITS(OOServer,OutputCDR)

using namespace Omega;
using namespace OTL;

Channel::Channel() :
	m_pManager(0),
	m_handle(ACE_INVALID_HANDLE)
{
}

void Channel::init(UserManager* pManager, ACE_HANDLE handle)
{
	if (m_pManager)
		OOSERVER_THROW_ERRNO(EALREADY);

	m_pManager = pManager;
	m_handle = handle;
}

Serialize::IFormattedStream* Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OOServer::OutputCDR
	ObjectPtr<ObjectImpl<OOServer::OutputCDR> > ptrOutput = ObjectImpl<OOServer::OutputCDR>::CreateObjectPtr(pOuter);
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
	ObjectPtr<ObjectImpl<OOServer::OutputCDR> > ptrOutput;
	ptrOutput.Attach(static_cast<ObjectImpl<OOServer::OutputCDR>*>(pStream->QueryInterface(OOServer::IID_OutputCDR)));
	if (!ptrOutput)
		OOSERVER_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = ptrOutput->GetMessageBlock();

	Serialize::IFormattedStream* pResponse = 0;
	try
	{
		if (attribs & Remoting::asynchronous)
		{
			if (m_pManager->send_asynch(m_handle,0,request,&deadline) != 0)
				OOSERVER_THROW_LASTERROR();
		}
		else
		{
			UserRequest* response = 0;
			if (m_pManager->send_synch(m_handle,0,request,response,&deadline) != 0)
				OOSERVER_THROW_LASTERROR();

			// Unpack and validate response...
			ACE_CDR::ULong ret_code = 0;
			if (!response->input()->read_ulong(ret_code))
				OOSERVER_THROW_LASTERROR();

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
			ObjectPtr<ObjectImpl<OOServer::InputCDR> > ptrResponse = ObjectImpl<OOServer::InputCDR>::CreateObjectPtr();
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
