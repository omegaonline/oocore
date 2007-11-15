///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 1999 Rick Taylor
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

ACE_CString User::string_t_to_utf8(const Omega::string_t& val)
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

User::Channel::Channel()
{
}

void User::Channel::init(ACE_CDR::UShort channel_id)
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
		OOSERVER_THROW_ERRNO(EINVAL);

	// Get the message block
	ACE_Message_Block* request = static_cast<ACE_Message_Block*>(ptrOutput->GetMessageBlock());

	ACE_InputCDR* response = 0;
	try
	{
		if (!User::Manager::USER_MANAGER::instance()->send_request(m_channel_id,request,response,timeout,attribs))
		{
			if (ACE_OS::last_error() == ENOENT)
			{
				void* TODO;  // Throw a remoting error
			}
			OOSERVER_THROW_LASTERROR();
		}
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

Omega::guid_t User::Channel::GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	// This must match OOCore::OID_ChannelMarshalFactory
	static guid_t oid = guid_t::FromString(L"{7E662CBB-12AF-4773-8B03-A1A82F7EBEF0}");

	return oid;
}

void User::Channel::MarshalInterface(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pStream->WriteUInt16(m_channel_id);
}

void User::Channel::ReleaseMarshalData(Omega::Remoting::IObjectManager*, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pStream->ReadUInt16();
}

OMEGA_DEFINE_OID(User,OID_ChannelMarshalFactory,"{1A7672C5-8478-4e5a-9D8B-D5D019E25D15}");

void User::ChannelMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t, Omega::IObject*& pObject)
{
	ACE_CDR::UShort channel_id = pStream->ReadUInt16();
	
	void* TODO;
}
