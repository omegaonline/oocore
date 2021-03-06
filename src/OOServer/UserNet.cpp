///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
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

#include "OOServer_User.h"
#include "UserManager.h"
#include "MessageConnection.h"

using namespace Omega;
using namespace OTL;

User::RemoteChannel::RemoteChannel() :
		m_channel_id(0),
		m_nNextChannelId(0)
{
}

ObjectImpl<User::Channel>* User::RemoteChannel::client_init(Remoting::IEndpoint* pEndpoint, const string_t& strEndpoint, uint32_t channel_id)
{
	m_channel_id = channel_id;

	// Open the remote endpoint and attach ourselves as the sink...
	m_ptrUpstream = pEndpoint->Open(strEndpoint,this);
	if (!m_ptrUpstream)
		OMEGA_THROW("IEndpoint::Open returned null sink");

	m_message_oid = pEndpoint->MessageOid();

	// Create a local channel around the new id (the remote channel will do the actual routing)
	return create_channel(0);
}

void User::RemoteChannel::server_init(Remoting::IChannelSink* pSink, const guid_t& message_oid, uint32_t channel_id)
{
	m_channel_id = channel_id;
	m_ptrUpstream = pSink;
	m_message_oid = message_oid;

	// Create a local channel around the new id (the remote channel will do the actual routing)
	create_channel(0);
}

ObjectImpl<User::Channel>* User::RemoteChannel::create_channel(uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	if (!m_mapChannels.find(channel_id,ptrChannel))
	{
		ptrChannel = ObjectImpl<User::Channel>::CreateObject();
		ptrChannel->init(m_channel_id | channel_id,Remoting::RemoteMachine,m_message_oid);

		int err = m_mapChannels.insert(channel_id,ptrChannel);
		if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrChannel.Detach();
}

Remoting::IObjectManager* User::RemoteChannel::create_object_manager(uint32_t channel_id)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel(channel_id);

	return ptrChannel->GetObjectManager();
}

void User::RemoteChannel::send_away(OOBase::CDRStream& msg, uint32_t src_channel_id, uint32_t dest_channel_id, uint32_t attribs, uint16_t dest_thread_id, uint16_t src_thread_id, OOServer::Message_t::Type type)
{
	// Make sure we have the source in the map...
	if (src_channel_id != 0)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		uint32_t channel_id = 0;
		if (m_mapChannelIds.find(src_channel_id,channel_id))
			src_channel_id = channel_id;
		else
		{
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			}
			while (!channel_id && m_mapChannelIds.exists(channel_id));

			int err = m_mapChannelIds.insert(src_channel_id,channel_id);
			if (err == 0)
			{
				err = m_mapChannelIds.insert(channel_id,src_channel_id);
				if (err != 0)
					m_mapChannelIds.remove(src_channel_id);
			}

			if (err != 0)
				OMEGA_THROW(err);

			// Add our channel id to the source
			src_channel_id = channel_id;
		}
	}

	// Trim high bits off destination
	dest_channel_id &= 0x000FFFFF;

	ObjectPtr<Remoting::IMessage> ptrPayload;

	// Custom handling of system messages
	if (attribs & OOServer::Message_t::system_message)
	{
		if (type == OOServer::Message_t::Request)
		{
			if ((attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect ||
				(attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
			{
				// Do nothing
			}
			else
				OMEGA_THROW("Invalid system message");
		}
		else
		{
			// Create a new message of the right format...
			if (m_message_oid == guid_t::Null())
				ptrPayload = ObjectImpl<OOCore::CDRMessage>::CreateObject();
			else
				ptrPayload = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

			if ((attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect)
			{
				// Unpack the channel_id
				OOBase::CDRStream input(msg);
				uint32_t channel_id;
				input.read(channel_id);
				if (input.last_error() != 0)
					OMEGA_THROW(input.last_error());

				// Write the channel id
				ptrPayload->WriteValue(string_t::constant("channel_id"),channel_id);
			}
			else if ((attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
			{
				// Write the pong
				ptrPayload->WriteValue(string_t::constant("pong"),byte_t(1));
			}
			else
				OMEGA_THROW("Invalid system message");
		}
	}
	else
	{
		// Unmarshal the payload
		if (msg.buffer()->length() > 0)
		{
			// Wrap the message block
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrInput = ObjectImpl<OOCore::CDRMessage>::CreateObject();
			ptrInput->init(msg);

			ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

			// QI for IMarshaller
			ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
			if (!ptrMarshaller)
				throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

			ptrPayload.Unmarshal(ptrMarshaller,string_t::constant("payload"),ptrInput);
		}
	}

	send_away_i(ptrPayload,src_channel_id,dest_channel_id,attribs,dest_thread_id,src_thread_id,type);
}

void User::RemoteChannel::send_away_i(Remoting::IMessage* pPayload, uint32_t src_channel_id, uint32_t dest_channel_id, uint32_t attribs, uint16_t dest_thread_id, uint16_t src_thread_id, OOServer::Message_t::Type type)
{
	// Create a new message of the right format...
	ObjectPtr<Remoting::IMessage> ptrMessage;
	if (m_message_oid == guid_t::Null())
		ptrMessage = ObjectImpl<OOCore::CDRMessage>::CreateObject();
	else
		ptrMessage = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

	// Write the mesage struct
	ptrMessage->WriteStructStart(string_t::constant("message"),string_t::constant("$rpc_msg"));
	ptrMessage->WriteValue(string_t::constant("src_channel_id"),src_channel_id);
	ptrMessage->WriteValue(string_t::constant("dest_channel_id"),dest_channel_id);
	ptrMessage->WriteValue(string_t::constant("attribs"),attribs);
	ptrMessage->WriteValue(string_t::constant("dest_thread_id"),dest_thread_id);
	ptrMessage->WriteValue(string_t::constant("src_thread_id"),src_thread_id);
	ptrMessage->WriteValue(string_t::constant("type"),type == OOServer::Message_t::Request ? true : false);

	// Get the source channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
	if (!ptrMarshaller)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

	ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);

	try
	{
		ptrMessage->WriteStructEnd();

		if (!m_ptrUpstream)
			throw Remoting::IChannelClosedException::Create();

		m_ptrUpstream->Send((TypeInfo::MethodAttributes_t)(attribs & 0xFFFF),ptrMessage);
	}
	catch (...)
	{
		ptrMessage->ReadStructStart(string_t::constant("message"),string_t::constant("$rpc_msg"));
		ptrMessage->ReadValue(string_t::constant("src_channel_id"));
		ptrMessage->ReadValue(string_t::constant("dest_channel_id"));
		ptrMessage->ReadValue(string_t::constant("attribs"));
		ptrMessage->ReadValue(string_t::constant("dest_thread_id"));
		ptrMessage->ReadValue(string_t::constant("src_thread_id"));
		ptrMessage->ReadValue(string_t::constant("type"));
		ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
		throw;
	}
}

void User::RemoteChannel::process_here(void* pParams, OOBase::CDRStream& input, OOBase::AllocatorInstance&)
{
	RemoteChannel* pThis = static_cast<RemoteChannel*>(pParams);

	try
	{
		pThis->process_here_i(input);
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
	}
	catch (...)
	{
		LOG_ERROR(("Unhandled exception thrown"));
	}

	pThis->Release();
}

void User::RemoteChannel::process_here_i(OOBase::CDRStream& input)
{
	// Read the header
	uint32_t src_channel_id;
	input.read(src_channel_id);
	uint32_t ex_attribs = 0;
	input.read(ex_attribs);
	uint16_t dest_thread_id;
	input.read(dest_thread_id);
	uint16_t src_thread_id;
	input.read(src_thread_id);
	if (input.last_error() != 0)
		OMEGA_THROW(input.last_error());

	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrMsg = ObjectImpl<OOCore::CDRMessage>::CreateObject();
	ptrMsg->init(input);

	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
	if (!ptrMarshaller)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload;
	ptrPayload.Unmarshal(ptrMarshaller,string_t::constant("payload"),ptrMsg);

	ObjectPtr<Remoting::IMessage> ptrResult = ptrOM->Invoke(ptrPayload);

	if (!(ex_attribs & OOServer::Message_t::asynchronous))
	{
		// Send it back...
		send_away_i(ptrResult,0,src_channel_id,OOServer::Message_t::synchronous,src_thread_id,dest_thread_id,OOServer::Message_t::Response);
	}
}

void User::RemoteChannel::Send(TypeInfo::MethodAttributes_t, Remoting::IMessage* pMsg)
{
	// This is a message from the other end...

	// Unpack parameters
	pMsg->ReadStructStart(string_t::constant("message"),string_t::constant("$rpc_msg"));
	uint32_t src_channel_id = pMsg->ReadValue(string_t::constant("src_channel_id")).cast<uint32_t>();
	uint32_t dest_channel_id = pMsg->ReadValue(string_t::constant("dest_channel_id")).cast<uint32_t>();
	uint32_t ex_attribs = pMsg->ReadValue(string_t::constant("attribs")).cast<uint32_t>();
	uint16_t dest_thread_id = pMsg->ReadValue(string_t::constant("dest_thread_id")).cast<uint16_t>();
	uint16_t src_thread_id = pMsg->ReadValue(string_t::constant("src_thread_id")).cast<uint16_t>();
	OOServer::Message_t::Type type = pMsg->ReadValue(string_t::constant("type")).cast<bool_t>() ? OOServer::Message_t::Request : OOServer::Message_t::Response;

	// Get the dest channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(dest_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller = ptrOM.QueryInterface<Remoting::IMarshaller>();
	if (!ptrMarshaller)
		throw OOCore_INotFoundException_MissingIID(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload;
	ptrPayload.Unmarshal(ptrMarshaller,string_t::constant("payload"),pMsg);

	pMsg->ReadStructEnd();

	if (!dest_channel_id)
	{
		if (ex_attribs & OOServer::Message_t::system_message)
		{
			if (type == OOServer::Message_t::Request)
			{
				ObjectPtr<Remoting::IMessage> ptrResult;
				uint32_t out_attribs = 0;

				switch (ex_attribs & OOServer::Message_t::system_message)
				{
				case OOServer::Message_t::channel_close:
					{
						uint32_t channel_id = ptrPayload->ReadValue(string_t::constant("channel_id")).cast<uint32_t>();

						Manager::instance()->channel_closed(channel_id | m_channel_id,0);

						out_attribs = OOServer::Message_t::asynchronous;
					}
					break;

				case OOServer::Message_t::channel_reflect:
					{
						// Create a new message of the right format...
						if (m_message_oid == guid_t::Null())
							ptrResult = ObjectImpl<OOCore::CDRMessage>::CreateObject();
						else
							ptrResult = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

						// Send back the src_channel_id
						ptrResult->WriteValue(string_t::constant("channel_id"),src_channel_id | m_channel_id);

						out_attribs = OOServer::Message_t::synchronous | OOServer::Message_t::channel_reflect;
					}
					break;

				case OOServer::Message_t::channel_ping:
					{
						// Create a new message of the right format...
						if (m_message_oid == guid_t::Null())
							ptrResult = ObjectImpl<OOCore::CDRMessage>::CreateObject();
						else
							ptrResult = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

						// Send back the pong
						ptrResult->WriteValue(string_t::constant("pong"),byte_t(1));

						out_attribs = OOServer::Message_t::synchronous | OOServer::Message_t::channel_ping;
					}
					break;

				default:
					OMEGA_THROW("Invalid system message");
				}

				if (!(out_attribs & OOServer::Message_t::asynchronous))
				{
					// Send it back...
					send_away_i(ptrResult,dest_channel_id,src_channel_id,out_attribs,src_thread_id,dest_thread_id,OOServer::Message_t::Response);
				}
			}
			else
				OMEGA_THROW("Invalid system message");
		}
		else
		{
			// Need to queue this as an async func...
			OOBase::CDRStream output;
			output.write(src_channel_id);
			output.write(ex_attribs);
			output.write(dest_thread_id);
			output.write(src_thread_id);
			output.write(type == OOServer::Message_t::Request ? true : false);
			if (output.last_error() != 0)
				OMEGA_THROW(output.last_error());

			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrMsg = ObjectImpl<OOCore::CDRMessage>::CreateObject();
			ptrMsg->init(output);

			ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);

			AddRef();

			void* TODO;
			// This seems broken...

			if (!Manager::instance()->call_async_function_i("process_here",&process_here,this,&output))
			{
				Release();

				ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
				OMEGA_THROW("Failed to queue message");
			}
		}
	}
	else
	{
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrOutput = ObjectImpl<OOCore::CDRMessage>::CreateObject();

		if (ex_attribs & OOServer::Message_t::system_message)
		{
			// Filter the system messages
			if (type == OOServer::Message_t::Request)
			{
				if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect ||
					(ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
				{
					// Pass on.. there is no payload to filter
				}
				else
					OMEGA_THROW("Invalid system message");
			}
			else
			{
				if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect)
				{
					// Unpack the channel_id
					uint32_t ch = ptrPayload->ReadValue(string_t::constant("channel_id")).cast<uint32_t>();

					// Repack in the right format
					ptrOutput->WriteValue(string_t::constant("channel_id"),ch);
				}
				else if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
				{
					// Unpack the pong
					byte_t p = ptrPayload->ReadValue(string_t::constant("pong")).cast<byte_t>();

					// Repack in the right format
					ptrOutput->WriteValue(string_t::constant("pong"),p);
				}
				else
					OMEGA_THROW("Invalid system message");
			}
		}
		else
		{
			// Marshal the message onto the CDR message
			ptrMarshaller->MarshalInterface(string_t::constant("payload"),ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
		}

		// Translate channel ids
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		uint32_t channel_id = 0;
		if (m_mapChannelIds.find(dest_channel_id,channel_id))
			dest_channel_id = channel_id;
		else
		{
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			}
			while (!channel_id && m_mapChannelIds.exists(channel_id));

			int err = m_mapChannelIds.insert(dest_channel_id,channel_id);
			if (err == 0)
			{
				err = m_mapChannelIds.insert(channel_id,dest_channel_id);
				if (err != 0)
					m_mapChannelIds.remove(dest_channel_id);
			}

			if (err != 0)
				OMEGA_THROW(err);

			// Add our channel id to the source
			dest_channel_id = channel_id;
		}

		guard.release();

		// Trim high bits off destination
		src_channel_id |= m_channel_id;

		// Forward through the network...
		OOServer::MessageHandler::io_result::type res = Manager::instance()->forward_message(src_channel_id,dest_channel_id,ex_attribs,dest_thread_id,src_thread_id,type,*ptrOutput->GetCDRStream());
		if (res != OOServer::MessageHandler::io_result::success)
		{
			if (!(ex_attribs & OOServer::Message_t::system_message))
				ptrMarshaller->ReleaseMarshalData(string_t::constant("payload"),ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);

			if (res == OOServer::MessageHandler::io_result::channel_closed)
				throw Remoting::IChannelClosedException::Create();
			else
				OMEGA_THROW("Internal server exception");
		}
	}
}

void User::RemoteChannel::channel_closed(uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	uint32_t i = 0;
	if (m_mapChannelIds.find(channel_id,i))
	{
		// Create a new message of the right format...
		ObjectPtr<Remoting::IMessage> ptrMsg;
		if (m_message_oid == guid_t::Null())
			ptrMsg = ObjectImpl<OOCore::CDRMessage>::CreateObject();
		else
			ptrMsg = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

		// Send back the src_channel_id
		ptrMsg->WriteValue(string_t::constant("channel_id"),i);

		guard.release();

		// Send a sys message
		send_away_i(ptrMsg,0,0,OOServer::Message_t::asynchronous | OOServer::Message_t::channel_close,0,0,OOServer::Message_t::Request);
	}
}

void User::RemoteChannel::Close()
{
	ObjectPtr<Remoting::IChannelSink> ptrUpstream;
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		ObjectPtr<ObjectImpl<Channel> > ptrChannel;
		while (m_mapChannels.pop(NULL,&ptrChannel))
			ptrChannel->disconnect();

		if (m_ptrUpstream)
		{
			ptrUpstream = m_ptrUpstream;
			m_ptrUpstream.Release();

			// Tell the manager that channels have closed
			for (size_t i=m_mapChannelIds.begin(); i!=m_mapChannelIds.npos; i=m_mapChannelIds.next(i))
			{
				uint32_t k = *m_mapChannelIds.key_at(i);
				if (k >= m_channel_id)
					break;

				Manager::instance()->channel_closed(k | m_channel_id,0);
			}

			Manager::instance()->channel_closed(m_channel_id,0);
		}
	}

	if (ptrUpstream)
		ptrUpstream->Close();
}

Remoting::IChannel* User::Manager::open_remote_channel(const string_t& strEndpoint)
{
	return s_instance->open_remote_channel_i(strEndpoint);
}

Remoting::IChannel* User::Manager::open_remote_channel_i(const string_t& strEndpoint)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(':');
	if (pos == string_t::npos)
		OMEGA_THROW("No protocol specified");

	string_t strProtocol = strEndpoint.Left(pos);

	// Look up handler in registry
	string_t strHandler;
	ObjectPtr<Omega::Registry::IKey> ptrKey(string_t::constant("Local User"));
	if (ptrKey->IsKey("Networking/Protocols/" + strProtocol))
	{
		ptrKey = ptrKey->OpenKey("Networking/Protocols/" + strProtocol);
		if (ptrKey->IsValue(string_t::constant("Endpoint")))
			strHandler = ptrKey->GetValue(string_t::constant("Endpoint")).cast<string_t>();
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(string_t::constant("System"));
		if (ptrKey->IsKey("Networking/Protocols/" + strProtocol))
		{
			ptrKey = ptrKey->OpenKey("Networking/Protocols/" + strProtocol);
			if (ptrKey->IsValue(string_t::constant("Endpoint")))
				strHandler = ptrKey->GetValue(string_t::constant("Endpoint")).cast<string_t>();
		}
	}

	// Create the factory
	ObjectPtr<Remoting::IEndpoint> ptrEndpoint(strHandler);

	// Create a new unique, upstream channel...
	RemoteChannelEntry channel;
	channel.strEndpoint = ptrEndpoint->Canonicalise(strEndpoint);

	// Check for duplicates
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_remote_lock);

		ObjectPtr<Remoting::IChannel> ptrChannel;
		if (m_mapRemoteChannels.find(channel.strEndpoint,ptrChannel))
			return ptrChannel.Detach();
	}

	// Create a sink for the new endpoint
	channel.ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateObject();

	// Lock from here on...
	OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

	uint32_t channel_id = 0;
	int err = m_mapRemoteChannelIds.insert(channel,channel_id,1,0x7fFFFFFF);
	if (err != 0)
		OMEGA_THROW(err);

	// Init the sink
	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	try
	{
		ptrChannel = channel.ptrRemoteChannel->client_init(ptrEndpoint,channel.strEndpoint,channel_id);
	}
	catch (...)
	{
		m_mapRemoteChannelIds.remove(channel_id);
		throw;
	}

	// Add to the maps
	err = m_mapRemoteChannels.insert(channel.strEndpoint,static_cast<Remoting::IChannel*>(ptrChannel));
	if (err != 0)
	{
		m_mapRemoteChannelIds.remove(channel_id);
		channel.ptrRemoteChannel->Close();
		OMEGA_THROW(err);
	}

	return ptrChannel.Detach();
}

void User::Manager::close_all_remotes()
{
	// Make a locked copy of the maps and close them

	OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

	RemoteChannelEntry channel;
	while (m_mapRemoteChannelIds.pop(NULL,&channel))
	{
		guard.release();

		try
		{
			channel.ptrRemoteChannel->Close();
		}
		catch (IException* pE)
		{
			ObjectPtr<IException> ptrE = pE;
			LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
		}

		guard.acquire();
	}
}

OOServer::MessageHandler::io_result::type User::Manager::route_off(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, OOServer::Message_t::Type type)
{
	try
	{
		ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel;
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

			RemoteChannelEntry channel_entry;
			if (!m_mapRemoteChannelIds.find(dest_channel_id & 0xFFF00000,channel_entry))
				return MessageHandler::route_off(msg,src_channel_id,dest_channel_id,attribs,dest_thread_id,src_thread_id,type);

			ptrRemoteChannel = channel_entry.ptrRemoteChannel;
		}

		// Send it on...
		ptrRemoteChannel->send_away(msg,src_channel_id,dest_channel_id,attribs,dest_thread_id,src_thread_id,type);

		return OOServer::MessageHandler::io_result::success;
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		return OOServer::MessageHandler::io_result::channel_closed;
	}
	catch (IException* pE)
	{
		ObjectPtr<IException> ptrE = pE;
		LOG_ERROR(("IException thrown: %s",recurse_log_exception(ptrE).c_str()));
		return OOServer::MessageHandler::io_result::failed;
	}
}

Remoting::IChannelSink* User::Manager::open_server_sink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return s_instance->open_server_sink_i(message_oid,pSink);
}

Remoting::IChannelSink* User::Manager::open_server_sink_i(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	// Create a sink for the new endpoint
	RemoteChannelEntry channel;
	channel.ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateObject();

	// Lock from here on...
	OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

	// Create a new unique, upstream channel...
	uint32_t channel_id = 0;
	int err = m_mapRemoteChannelIds.insert(channel,channel_id,1,0x7fFFFFFF);
	if (err != 0)
		OMEGA_THROW(err);

	try
	{
		// Init the sink
		channel.ptrRemoteChannel->server_init(pSink,message_oid,channel_id);
	}
	catch (...)
	{
		m_mapRemoteChannelIds.remove(channel_id);
		throw;
	}

	return channel.ptrRemoteChannel.AddRef();
}

void User::Manager::local_channel_closed(OOBase::Stack<uint32_t,OOBase::AllocatorInstance>& channels)
{
	// Local channels have closed
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_remote_lock);

	for (uint32_t j = 0;channels.pop(&j);)
	{
		for (size_t i=m_mapRemoteChannelIds.begin(); i!=m_mapRemoteChannelIds.npos;i=m_mapRemoteChannelIds.next(i))
		{
			if ((j & 0xFFF00000) != *m_mapRemoteChannelIds.key_at(i))
				m_mapRemoteChannelIds.at(i)->ptrRemoteChannel->channel_closed(j);
		}
	}
}
