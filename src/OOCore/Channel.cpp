#include "OOCore_precomp.h"

#include "./Channel.h"
#include "./UserSession.h"

using namespace Omega;
using namespace OTL;

ACE_CString OOCore::string_t_to_utf8(const Omega::string_t& val)
{
	ACE_CString str;
	char szBuf[256];
	size_t len = val.ToUTF8(szBuf,256);
	if (len > 256)
	{
		char* pszBuf;
		OMEGA_NEW(pszBuf,char[len]);
		val.ToUTF8(pszBuf,len);
		str = pszBuf;
		delete [] pszBuf;
	}
	else
		str = szBuf;

	return str;
}

OOCore::Channel::Channel() :
	m_pSession(0), m_thread_id(0)
{
}

void OOCore::Channel::init(UserSession* pSession, ACE_CDR::UShort channel_id)
{
	if (m_pSession)
		OOCORE_THROW_ERRNO(EALREADY);

	m_pSession = pSession;
	m_channel_id = channel_id;
}

ACE_CDR::UShort OOCore::Channel::set_thread_id(ACE_CDR::UShort thread_id)
{
	ACE_CDR::UShort prev = *m_thread_id;
	*m_thread_id = thread_id;
	return prev;
}

Serialize::IFormattedStream* OOCore::Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr(pOuter);
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(OMEGA_UUIDOF(Omega::Serialize::IFormattedStream)));
}

Serialize::IFormattedStream* OOCore::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pStream, uint16_t timeout)
{
	// QI pStream for our private interface
	ObjectPtr<IOutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<IOutputCDR*>(pStream->QueryInterface(OMEGA_UUIDOF(IOutputCDR))));
	if (!ptrOutput)
		OOCORE_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	Serialize::IFormattedStream* pResponse = 0;
	ACE_InputCDR* response = 0;
	try
	{
		if (!m_pSession->send_request(m_channel_id,*m_thread_id,request,response,timeout,attribs))
			OOCORE_THROW_LASTERROR();
		
		if (response)
		{
			// Unpack and validate response...
			ACE_CDR::Octet ret_code = 0;
			if (!response->read_octet(ret_code))
				OOCORE_THROW_LASTERROR();

			// ret_code must match the values in UserSession::process_request
			if (ret_code == 1)
			{
				OMEGA_THROW(L"Request timed out");
			}
			else if (ret_code == 2)
			{
				ACE_CString strDesc;
				if (!response->read_string(strDesc))
					OOCORE_THROW_LASTERROR();

				ACE_CString strSrc;
				if (!response->read_string(strSrc))
					OOCORE_THROW_LASTERROR();

				throw IException::Create(string_t(strDesc.c_str(),true),string_t(strSrc.c_str(),true));
			}
			
			// Wrap the response
			ObjectPtr<ObjectImpl<InputCDR> > ptrResponse = ObjectImpl<InputCDR>::CreateInstancePtr();
			ptrResponse->init(*response);
			response = 0;
			pResponse = ptrResponse.Detach();
		}
	}
	catch (...)
	{
		delete response;
		request->release();
		throw;
	}

	request->release();
	return pResponse;
}
