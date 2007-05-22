#include "OOServer.h"

#include "./Channel.h"
#include "./UserManager.h"

using namespace Omega;
using namespace OTL;

User::Channel::Channel() :
	m_pManager(0),
	m_handle(ACE_INVALID_HANDLE)
{
}

void User::Channel::init(Manager* pManager, ACE_HANDLE handle, ACE_CDR::UShort channel_id)
{
	if (m_pManager)
		OOSERVER_THROW_ERRNO(EALREADY);

	m_pManager = pManager;
	m_handle = handle;
	m_channel_id = channel_id;
}

Serialize::IFormattedStream* User::Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr(pOuter);
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(OMEGA_UUIDOF(Omega::Serialize::IFormattedStream)));
}

Serialize::IFormattedStream* User::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pStream)
{
	// TODO We need to make the timeout cumulative - i.e. catch the first request, and use a kind
	// of 'time remaining' value to force all calls to occur within the timeout of the
	// outermost requests timeout...
	void* TODO;

	ACE_Time_Value deadline = ACE_OS::gettimeofday() + ACE_Time_Value(30);
	
	// QI pStream for our private interface
	ObjectPtr<IOutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<IOutputCDR*>(pStream->QueryInterface(OMEGA_UUIDOF(IOutputCDR))));
	if (!ptrOutput)
		OOSERVER_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	Serialize::IFormattedStream* pResponse = 0;
	try
	{
		if (attribs & Remoting::asynchronous)
		{
			m_pManager->send_asynch(m_handle,m_channel_id,request,&deadline);
		}
		else
		{
			ACE_InputCDR response = m_pManager->send_synch(m_handle,m_channel_id,request,&deadline);

			// Unpack and validate response...
			ACE_CDR::Octet ret_code = 0;
			if (!response.read_octet(ret_code))
				OOSERVER_THROW_LASTERROR();

			// ret_code must match the values in UserManager::process_request
			if (ret_code == 1)
			{
				OMEGA_THROW("Request timed out");
			}
			else if (ret_code == 2)
			{
				ACE_CString strDesc;
				if (!response.read_string(strDesc))
					OOSERVER_THROW_LASTERROR();

				ACE_CString strSrc;
				if (!response.read_string(strSrc))
					OOSERVER_THROW_LASTERROR();

				throw IException::Create(strDesc.c_str(),strSrc.c_str());
			}
						
			// Wrap the response
			ObjectPtr<ObjectImpl<InputCDR> > ptrResponse = ObjectImpl<InputCDR>::CreateInstancePtr();
			ptrResponse->init(response);
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
