///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer.h"

#include "./Channel.h"
#include "./UserManager.h"

using namespace Omega;
using namespace OTL;

User::Channel::Channel()
{
}

void User::Channel::init(ACE_CDR::ULong channel_id)
{
	m_channel_id = channel_id;
}

Serialize::IFormattedStream* User::Channel::CreateOutputStream()
{
	// Create a fresh OutputCDR
	ObjectPtr<ObjectImpl<OutputCDR> > ptrOutput = ObjectImpl<OutputCDR>::CreateInstancePtr();
	return static_cast<Serialize::IFormattedStream*>(ptrOutput->QueryInterface(OMEGA_UUIDOF(Omega::Serialize::IFormattedStream)));
}

IException* User::Channel::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv, uint16_t timeout)
{
	// QI pStream for our private interface
	ObjectPtr<IOutputCDR> ptrOutput;
	ptrOutput.Attach(static_cast<IOutputCDR*>(pSend->QueryInterface(OMEGA_UUIDOF(IOutputCDR))));
	if (!ptrOutput)
		OMEGA_THROW(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	ACE_InputCDR* response = 0;
	if (!User::Manager::USER_MANAGER::instance()->send_request(m_channel_id,request,response,timeout,attribs))
	{
		request->release();
		OOSERVER_THROW_LASTERROR();
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
				OOSERVER_THROW_LASTERROR();

			// ret_code must match the values in UserManager::process_user_request
			if (ret_code == 1)
			{
				ACE_CString strDesc;
				if (!response->read_string(strDesc))
					OOSERVER_THROW_LASTERROR();

				ACE_CString strSrc;
				if (!response->read_string(strSrc))
					OOSERVER_THROW_LASTERROR();

				throw ISystemException::Create(string_t(strDesc.c_str(),true),string_t(strSrc.c_str(),true));
			}

			// Wrap the response
			ObjectPtr<ObjectImpl<InputCDR> > ptrRecv = ObjectImpl<InputCDR>::CreateInstancePtr();
			ptrRecv->init(*response);
			delete response;
			pRecv = ptrRecv.Detach();
		}
	}
	catch (...)
	{
		delete response;
		throw;
	}

	return 0;
}

OMEGA_DEFINE_OID(User,OID_ChannelMarshalFactory,"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");

Omega::guid_t User::Channel::GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	static guid_t oid = guid_t::FromString(L"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");
	return oid;
}

void User::Channel::MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	pStream->WriteUInt32(m_channel_id);
}

void User::Channel::ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	pStream->ReadUInt32();
}

void User::ChannelMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject)
{
	// We are unmarshalling a channel from another process...
	ACE_CDR::ULong channel_id = pStream->ReadUInt32();

	// Create a new object manager (and channel)
	pObject = User::Manager::USER_MANAGER::instance()->create_object_manager(channel_id,flags)->QueryInterface(iid);
}
