///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "./Channel.h"
#include "./UserSession.h"

using namespace Omega;
using namespace OTL;

OOCore::Channel::Channel() :
	m_channel_id(0),
	m_marshal_flags(0)
{
}

void OOCore::Channel::init(ACE_CDR::ULong channel_id, Remoting::MarshalFlags_t marshal_flags)
{
	m_channel_id = channel_id;
	m_marshal_flags = marshal_flags;
}

void OOCore::Channel::disconnect()
{
	m_channel_id = 0;
}

IO::IFormattedStream* OOCore::Channel::CreateOutputStream()
{
	if (!m_channel_id)
		OMEGA_THROW(ECONNRESET);

	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr();
	return ptrOutput.QueryInterface<IO::IFormattedStream>();
}

IException* OOCore::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, IO::IFormattedStream* pSend, IO::IFormattedStream*& pRecv,  uint16_t timeout)
{
	if (!m_channel_id)
		OMEGA_THROW(ECONNRESET);

	// QI pStream for our private interface
	ObjectPtr<IOutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<IOutputCDR*>(pSend->QueryInterface(OMEGA_UUIDOF(IOutputCDR))));
	if (!ptrOutput)
		OMEGA_THROW(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	ACE_InputCDR* response = 0;
	if (!UserSession::USER_SESSION::instance()->send_request(m_channel_id,request,response,timeout,attribs))
	{
		request->release();
		OOCORE_THROW_LASTERROR();
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

				throw ISystemException::Create(string_t(strDesc.c_str(),true),string_t(strSrc.c_str(),true));
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

Remoting::MarshalFlags_t OOCore::Channel::GetMarshalFlags()
{
	return m_marshal_flags;
}

uint32_t OOCore::Channel::GetSource()
{
	return m_channel_id;
}

guid_t OOCore::Channel::GetUnmarshalFactoryOID(const guid_t&, Remoting::MarshalFlags_t flags)
{
	// We cannot custom marshal to another machine
	if (flags == Remoting::another_machine)
		return guid_t::Null();

	return OID_ChannelMarshalFactory;
}

void OOCore::Channel::MarshalInterface(Remoting::IObjectManager*, IO::IFormattedStream* pStream, const guid_t&, Remoting::MarshalFlags_t)
{
	pStream->WriteUInt32(m_channel_id);
}

void OOCore::Channel::ReleaseMarshalData(Remoting::IObjectManager*, IO::IFormattedStream* pStream, const guid_t&, Remoting::MarshalFlags_t)
{
	pStream->ReadUInt32();
}

OMEGA_DEFINE_OID(OOCore,OID_ChannelMarshalFactory,"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

void OOCore::ChannelMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, IO::IFormattedStream* pStream, const guid_t& iid, Remoting::MarshalFlags_t flags, IObject*& pObject)
{
	try
	{
		// This must match OOServer::User::OID_ChannelMarshalFactory
		static const guid_t oid = guid_t::FromString(L"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");
		ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess | Activation::DontLaunch);

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
	ACE_CDR::ULong channel_id = pStream->ReadUInt32();
	
	// Create a new object manager (and channel)
	pObject = UserSession::USER_SESSION::instance()->create_object_manager(channel_id)->QueryInterface(iid);
}
