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

OOCore::Channel::Channel()
{
}

void OOCore::Channel::init(ACE_CDR::UShort channel_id)
{
	m_channel_id = channel_id;
}

Omega::Serialize::IFormattedStream* OOCore::Channel::CreateOutputStream(IObject* pOuter)
{
	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr(pOuter);
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(OMEGA_UUIDOF(Omega::Serialize::IFormattedStream)));
}

IException* OOCore::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv,  uint16_t timeout)
{
	// QI pStream for our private interface
	ObjectPtr<IOutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<IOutputCDR*>(pSend->QueryInterface(OMEGA_UUIDOF(IOutputCDR))));
	if (!ptrOutput)
		OOCORE_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	ACE_InputCDR* response = 0;
	try
	{
		if (!UserSession::USER_SESSION::instance()->send_request(m_channel_id,request,response,timeout,attribs))
			OOCORE_THROW_LASTERROR();
	}
	catch (...)
	{
		delete response;
		request->release();
		throw;
	}

	// Done with request
	request->release();

	try
	{
		if (response)
		{
			// Unpack and validate response...
			ACE_CDR::Octet ret_code = 0;
			if (!response->read_octet(ret_code))
				OOCORE_THROW_LASTERROR();

			// ret_code must match the values in UserSession::process_request
			if (ret_code == 1)
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
			ObjectPtr<ObjectImpl<InputCDR> > ptrRecv = ObjectImpl<InputCDR>::CreateInstancePtr();
			ptrRecv->init(*response);
			delete response;
			pRecv = ptrRecv.Detach();
		}
	}
	catch (IException* pE)
	{
		delete response;
		return pE;
	}
	catch (...)
	{
		delete response;
		throw;
	}

	return 0;
}

Omega::guid_t OOCore::Channel::GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	return OID_ChannelMarshalFactory;
}

void OOCore::Channel::MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pStream->WriteUInt16(m_channel_id);
}

void OOCore::Channel::ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pStream->ReadUInt16();
}

OMEGA_DEFINE_OID(OOCore,OID_ChannelMarshalFactory,"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

void OOCore::ChannelMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t flags, Omega::IObject*& pObject)
{
	try
	{
		// This must match OOServer::User::OID_ChannelMarshalFactory
		static guid_t oid = guid_t::FromString(L"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");

		ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess);

		// If we have a pointer by now then we are actually running in the OOServer.exe, 
		// and can therefore do our specialized unmarshalling...
		return ptrMarshalFactory->UnmarshalInterface(pObjectManager,pStream,iid,flags,pObject);
	}
	catch (Activation::IOidNotFoundException* pE)
	{
		// We can continue if this happens
		pE->Release();
	}
	
	// If we get here, then we are loaded into a different exe from OOServer,
	// therefore we do simple unmarshalling

	ACE_CDR::UShort channel_id = pStream->ReadUInt16();

	// Create a new object manager (and channel)
	pObject = UserSession::USER_SESSION::instance()->get_object_manager(channel_id)->QueryInterface(iid);
}
