#include "OOServer.h"

#include "./Channel.h"
#include "./UserManager.h"

OMEGA_EXPORT_INTERFACE_DERIVED
(
	OOServer, OutputCDR, Omega::Serialize, IFormattedStream, 
	0x5e8c6ed6, 0xe8b7, 0x4bc6, 0xa2, 0x65, 0x79, 0xe3, 0xb5, 0x51, 0xa4, 0x3e,

	// Methods
	OMEGA_METHOD(void*,GetMessageBlock,0,())
)
const Omega::guid_t OOServer::IID_OutputCDR = { 0x5e8c6ed6, 0xe8b7, 0x4bc6, { 0xa2, 0x65, 0x79, 0xe3, 0xb5, 0x51, 0xa4, 0x3e } };

namespace Omega
{
	namespace MetaInfo
	{
		OMEGA_QI_MAGIC(OOServer,OutputCDR)
	}
}

using namespace Omega;
using namespace OTL;

Channel::Channel() :
	m_pManager(0),
	m_handle(ACE_INVALID_HANDLE)
{
}

void Channel::init(UserManager* pManager, ACE_HANDLE handle, ACE_CDR::UShort channel_id)
{
	if (m_pManager)
		OOSERVER_THROW_ERRNO(EALREADY);

	m_pManager = pManager;
	m_handle = handle;
	m_channel_id = channel_id;
}

Serialize::IFormattedStream* Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OutputCDRImpl
	ObjectPtr<ObjectImpl<OutputCDRImpl> > ptrOutput = ObjectImpl<OutputCDRImpl>::CreateObjectPtr(pOuter);
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
	ObjectPtr<OOServer::OutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<OOServer::OutputCDR*>(pStream->QueryInterface(OOServer::IID_OutputCDR)));
	if (!ptrOutput)
		OOSERVER_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	Serialize::IFormattedStream* pResponse = 0;
	try
	{
		if (attribs & Remoting::asynchronous)
		{
			if (m_pManager->send_asynch(m_handle,m_channel_id,request,&deadline) != 0)
				OOSERVER_THROW_LASTERROR();
		}
		else
		{
			UserRequest* response = 0;
			if (m_pManager->send_synch(m_handle,m_channel_id,request,response,&deadline) != 0)
				OOSERVER_THROW_LASTERROR();

			// Unpack and validate response...
			ACE_CDR::Octet ret_code = 0;
			if (!response->input()->read_octet(ret_code))
				OOSERVER_THROW_LASTERROR();

			// ret_code must match the values in UserManager::process_request
			if (ret_code == 1)
			{
				OMEGA_THROW("Request timed out");
			}
			else if (ret_code == 2)
			{
				ACE_CString strDesc;
				if (!response->input()->read_string(strDesc))
					OOSERVER_THROW_LASTERROR();

				ACE_CString strSrc;
				if (!response->input()->read_string(strSrc))
					OOSERVER_THROW_LASTERROR();

				IException::Throw(strDesc.c_str(),strSrc.c_str());
			}
						
			// Wrap the response
			ObjectPtr<ObjectImpl<InputCDR> > ptrResponse = ObjectImpl<InputCDR>::CreateObjectPtr();
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
