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
		m_pManager(0),
		m_channel_id(0),
		m_nNextChannelId(0)
{
}

ObjectPtr<ObjectImpl<User::Channel> > User::RemoteChannel::client_init(Manager* pManager, Remoting::IEndpoint* pEndpoint, const string_t& strEndpoint, uint32_t channel_id)
{
	m_pManager = pManager;
	m_channel_id = channel_id;

	// Open the remote endpoint and attach ourselves as the sink...
	m_ptrUpstream.Attach(pEndpoint->Open(strEndpoint,this));
	if (!m_ptrUpstream)
		OMEGA_THROW("IEndpoint::Open returned null sink");

	m_message_oid = pEndpoint->MessageOid();

	// Create a local channel around the new id (the remote channel will do the actual routing)
	return create_channel(0);
}

void User::RemoteChannel::server_init(Manager* pManager, Remoting::IChannelSink* pSink, const guid_t& message_oid, uint32_t channel_id)
{
	m_pManager = pManager;
	m_channel_id = channel_id;
	m_ptrUpstream = pSink;
	m_message_oid = message_oid;

	// Create a local channel around the new id (the remote channel will do the actual routing)
	create_channel(0);
}

ObjectPtr<ObjectImpl<User::Channel> > User::RemoteChannel::create_channel(uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	if (!m_mapChannels.find(channel_id,ptrChannel))
	{
		ptrChannel = ObjectImpl<User::Channel>::CreateInstancePtr();
		ptrChannel->init(m_pManager,m_channel_id | channel_id,Remoting::RemoteMachine,m_message_oid);

		int err = m_mapChannels.insert(channel_id,ptrChannel);
		if (err != 0)
			OMEGA_THROW(err);
	}

	return ptrChannel;
}

ObjectPtr<Remoting::IObjectManager> User::RemoteChannel::create_object_manager(uint32_t channel_id)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel(channel_id);

	return ptrChannel->GetObjectManager();
}

void User::RemoteChannel::send_away(OOBase::CDRStream& msg, uint32_t src_channel_id, uint32_t dest_channel_id, const OOBase::timeval_t& deadline, uint32_t attribs, uint16_t dest_thread_id, uint16_t src_thread_id, uint16_t flags, uint32_t seq_no)
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
					m_mapChannelIds.erase(src_channel_id);
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
		if (flags == OOServer::Message_t::Request)
		{
			if ((attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect ||
				(attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
			{
				// Do nothing
			}
			else
				OMEGA_THROW("Invalid system message");
		}
		else if (flags == OOServer::Message_t::Response)
		{
			// Create a new message of the right format...
			if (m_message_oid == guid_t::Null())
				ptrPayload.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
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
				ptrPayload->WriteValue(L"channel_id",channel_id);
			}
			else if ((attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
			{
				// Write the pong
				ptrPayload->WriteValue(L"pong",byte_t(1));
			}
			else
				OMEGA_THROW("Invalid system message");
		}
		else
			OMEGA_THROW("Invalid system message");
	}
	else
	{
		// Unmarshal the payload
		if (msg.buffer()->length() > 0)
		{
			// Wrap the message block
			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrInput = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
			ptrInput->init(msg);

			ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

			// QI for IMarshaller
			ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
			if (!ptrMarshaller)
				throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

			ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrInput);
		}
	}

	send_away_i(ptrPayload,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
}

void User::RemoteChannel::send_away_i(Remoting::IMessage* pPayload, uint32_t src_channel_id, uint32_t dest_channel_id, const OOBase::timeval_t& deadline, uint32_t attribs, uint16_t dest_thread_id, uint16_t src_thread_id, uint16_t flags, uint32_t seq_no)
{
	// Create a new message of the right format...
	ObjectPtr<Remoting::IMessage> ptrMessage;
	if (m_message_oid == guid_t::Null())
		ptrMessage.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
	else
		ptrMessage = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

	// Write the mesage struct
	ptrMessage->WriteStructStart(L"message",L"$rpc_msg");
	ptrMessage->WriteValue(L"src_channel_id",src_channel_id);
	ptrMessage->WriteValue(L"dest_channel_id",dest_channel_id);

	// Fiddle with remote deadline value...
	int64_t secs = 0;
	int32_t usecs = 0;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		secs = deadline.tv_sec();
		usecs = deadline.tv_usec();
	}
	ptrMessage->WriteValue(L"deadline_secs",secs);
	ptrMessage->WriteValue(L"deadline_usecs",usecs);
	ptrMessage->WriteValue(L"attribs",attribs);
	ptrMessage->WriteValue(L"dest_thread_id",dest_thread_id);
	ptrMessage->WriteValue(L"src_thread_id",src_thread_id);
	ptrMessage->WriteValue(L"flags",flags);
	ptrMessage->WriteValue(L"seq_no",seq_no);

	// Get the source channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	ptrMarshaller->MarshalInterface(L"payload",ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);

	try
	{
		ptrMessage->WriteStructEnd();

		uint32_t timeout = 0;
		if (deadline != OOBase::timeval_t::MaxTime)
		{
			OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
			if (deadline <= now)
				throw ITimeoutException::Create();

			timeout = (deadline - now).msec();
		}

		if (!m_ptrUpstream)
			throw Remoting::IChannelClosedException::Create();

		m_ptrUpstream->Send((TypeInfo::MethodAttributes_t)(attribs & 0xFFFF),ptrMessage,timeout);
	}
	catch (...)
	{
		ptrMessage->ReadStructStart(L"message",L"$rpc_msg");
		ptrMessage->ReadValue(L"src_channel_id");
		ptrMessage->ReadValue(L"dest_channel_id");
		ptrMessage->ReadValue(L"deadline_secs");
		ptrMessage->ReadValue(L"deadline_usecs");
		ptrMessage->ReadValue(L"attribs");
		ptrMessage->ReadValue(L"dest_thread_id");
		ptrMessage->ReadValue(L"src_thread_id");
		ptrMessage->ReadValue(L"flags");
		ptrMessage->ReadValue(L"seq_no");
		ptrMarshaller->ReleaseMarshalData(L"payload",ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
		throw;
	}
}

void User::RemoteChannel::process_here(void* pParams, OOBase::CDRStream& input)
{
	RemoteChannel* pThis = static_cast<RemoteChannel*>(pParams);

	try
	{
		pThis->process_here_i(input);
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
		pE->Release();
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
	int64_t secs = 0;
	input.read(secs);
	int32_t usecs = 0;
	input.read(usecs);

	OOBase::timeval_t deadline = OOBase::timeval_t::MaxTime;
	if (secs != 0 && usecs != 0)
		deadline = OOBase::timeval_t(secs,usecs);

	uint32_t ex_attribs = 0;
	input.read(ex_attribs);
	uint16_t dest_thread_id;
	input.read(dest_thread_id);
	uint16_t src_thread_id;
	input.read(src_thread_id);
	uint32_t seq_no;
	input.read(seq_no);
	if (input.last_error() != 0)
		OMEGA_THROW(input.last_error());

	ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrMsg = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
	ptrMsg->init(input);

	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrMsg);

	// Check timeout
	uint32_t timeout = 0;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::timeval_t::gettimeofday();
		if (deadline <= now)
			return;

		timeout = (deadline - now).msec();
	}

	ObjectPtr<Remoting::IMessage> ptrResult;
	ptrResult.Attach(ptrOM->Invoke(ptrPayload,timeout));

	if (!(ex_attribs & OOServer::Message_t::asynchronous))
	{
		if (deadline != OOBase::timeval_t::MaxTime)
		{
			if (deadline <= OOBase::timeval_t::gettimeofday())
				return;
		}

		// Send it back...
		send_away_i(ptrResult,0,src_channel_id,deadline,OOServer::Message_t::synchronous,src_thread_id,dest_thread_id,OOServer::Message_t::Response,seq_no);
	}
}

void User::RemoteChannel::Send(TypeInfo::MethodAttributes_t, Remoting::IMessage* pMsg, uint32_t timeout)
{
	// This is a message from the other end...

	// Unpack parameters
	pMsg->ReadStructStart(L"message",L"$rpc_msg");
	uint32_t src_channel_id = pMsg->ReadValue(L"src_channel_id").cast<uint32_t>();
	uint32_t dest_channel_id = pMsg->ReadValue(L"dest_channel_id").cast<uint32_t>();
	int64_t secs = pMsg->ReadValue(L"deadline_secs").cast<int64_t>();
	int32_t usecs = pMsg->ReadValue(L"deadline_usecs").cast<int32_t>();
	uint32_t ex_attribs = pMsg->ReadValue(L"attribs").cast<uint32_t>();

	OOBase::timeval_t deadline = OOBase::timeval_t::MaxTime;
	if (secs != 0 && usecs != 0)
		deadline = OOBase::timeval_t(secs,usecs);

	if (timeout != 0)
	{
		OOBase::timeval_t tdeadline = OOBase::timeval_t::deadline(timeout);
		if (tdeadline < deadline)
			deadline = tdeadline;
	}

	// Fiddle with remote deadline value...
	uint16_t dest_thread_id = pMsg->ReadValue(L"dest_thread_id").cast<uint16_t>();
	uint16_t src_thread_id = pMsg->ReadValue(L"src_thread_id").cast<uint16_t>();
	uint16_t flags = pMsg->ReadValue(L"flags").cast<uint16_t>();
	uint32_t seq_no = pMsg->ReadValue(L"seq_no").cast<uint32_t>();

	// Get the dest channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(dest_channel_id);

	// QI for IMarshaller
	ObjectPtr<Remoting::IMarshaller> ptrMarshaller(ptrOM);
	if (!ptrMarshaller)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller));

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",pMsg);

	pMsg->ReadStructEnd();

	if (!dest_channel_id)
	{
		if (ex_attribs & OOServer::Message_t::system_message)
		{
			if (flags == OOServer::Message_t::Request)
			{
				ObjectPtr<Remoting::IMessage> ptrResult;
				uint32_t out_attribs = 0;

				if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_close)
				{
					uint32_t channel_id = ptrPayload->ReadValue(L"channel_id").cast<uint32_t>();

					m_pManager->channel_closed(channel_id | m_channel_id,0);

					out_attribs = OOServer::Message_t::asynchronous;
				}
				else if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect)
				{
					// Create a new message of the right format...
					if (m_message_oid == guid_t::Null())
						ptrResult.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
					else
						ptrResult = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

					// Send back the src_channel_id
					ptrResult->WriteValue(L"channel_id",src_channel_id | m_channel_id);

					out_attribs = OOServer::Message_t::synchronous | OOServer::Message_t::channel_reflect;
				}
				else if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
				{
					// Create a new message of the right format...
					if (m_message_oid == guid_t::Null())
						ptrResult.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
					else
						ptrResult = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

					// Send back the pong
					ptrResult->WriteValue(L"pong",byte_t(1));

					out_attribs = OOServer::Message_t::synchronous | OOServer::Message_t::channel_ping;
				}
				else
					OMEGA_THROW("Invalid system message");

				if (!(out_attribs & OOServer::Message_t::asynchronous))
				{
					// Send it back...
					send_away_i(ptrResult,dest_channel_id,src_channel_id,deadline,out_attribs,src_thread_id,dest_thread_id,OOServer::Message_t::Response,seq_no);
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
			int64_t secs = 0;
			int32_t usecs = 0;
			if (deadline != OOBase::timeval_t::MaxTime)
			{
				secs = deadline.tv_sec();
				usecs = deadline.tv_usec();
			}
			output.write(secs);
			output.write(usecs);
			output.write(ex_attribs);
			output.write(dest_thread_id);
			output.write(src_thread_id);
			output.write(seq_no);
			if (output.last_error() != 0)
				OMEGA_THROW(output.last_error());

			ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrMsg = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();
			ptrMsg->init(output);

			ptrMarshaller->MarshalInterface(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);

			AddRef();

			if (!m_pManager->call_async_function_i("process_here",&process_here,this,&output))
			{
				Release();

				ptrMarshaller->ReleaseMarshalData(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
				OMEGA_THROW("Failed to queue message");
			}
		}
	}
	else
	{
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrOutput = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();

		if (ex_attribs & OOServer::Message_t::system_message)
		{
			// Filter the system messages
			if (flags == OOServer::Message_t::Request)
			{
				if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect ||
					(ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
				{
					// Pass on.. there is no payload to filter
				}
				else
					OMEGA_THROW("Invalid system message");
			}
			else if (flags == OOServer::Message_t::Response)
			{
				if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_reflect)
				{
					// Unpack the channel_id
					uint32_t ch = ptrPayload->ReadValue(L"channel_id").cast<uint32_t>();

					// Repack in the right format
					ptrOutput->WriteValue(L"channel_id",ch);
				}
				else if ((ex_attribs & OOServer::Message_t::system_message) == OOServer::Message_t::channel_ping)
				{
					// Unpack the pong
					byte_t p = ptrPayload->ReadValue(L"pong").cast<byte_t>();

					// Repack in the right format
					ptrOutput->WriteValue(L"pong",p);
				}
				else
					OMEGA_THROW("Invalid system message");
			}
			else
				OMEGA_THROW("Invalid system message");
		}
		else
		{
			// Marshal the message onto the CDR message
			ptrMarshaller->MarshalInterface(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
		}

		// Translate channel ids
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		uint32_t channel_id = 0;
		if (m_mapChannelIds.find(dest_channel_id,channel_id))
			dest_channel_id = channel_id;
		else
		{
			uint32_t channel_id = 0;
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
					m_mapChannelIds.erase(dest_channel_id);
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
		OOServer::MessageHandler::io_result::type res = m_pManager->forward_message(src_channel_id,dest_channel_id,deadline,ex_attribs,dest_thread_id,src_thread_id,flags,seq_no,*ptrOutput->GetCDRStream());
		if (res != OOServer::MessageHandler::io_result::success)
		{
			if (!(ex_attribs & OOServer::Message_t::system_message))
				ptrMarshaller->ReleaseMarshalData(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);

			if (res == OOServer::MessageHandler::io_result::timedout)
				throw ITimeoutException::Create();
			else if (res == OOServer::MessageHandler::io_result::channel_closed)
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
			ptrMsg.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
		else
			ptrMsg = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::Library);

		// Send back the src_channel_id
		ptrMsg->WriteValue(L"channel_id",i);

		guard.release();

		// Send a sys message
		send_away_i(ptrMsg,0,0,OOBase::timeval_t::MaxTime,OOServer::Message_t::asynchronous | OOServer::Message_t::channel_close,0,0,OOServer::Message_t::Request,0);
	}
}

void User::RemoteChannel::Close()
{
	OTL::ObjectPtr<Omega::Remoting::IChannelSink> ptrUpstream;
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

				m_pManager->channel_closed(k | m_channel_id,0);
			}

			m_pManager->channel_closed(m_channel_id,0);
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
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW("No protocol specified");

	string_t strProtocol = strEndpoint.Left(pos).ToLower();

	// Look up handler in registry
	string_t strHandler;
	ObjectPtr<Registry::IKey> ptrKey(L"Local User");
	if (ptrKey->IsSubKey(L"Networking/Protocols/" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking/Protocols/" + strProtocol);
		if (ptrKey->IsValue(L"Endpoint"))
			strHandler = ptrKey->GetValue(L"Endpoint").cast<string_t>();
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Registry::IKey>(L"System");
		if (ptrKey->IsSubKey(L"Networking/Protocols/" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking/Protocols/" + strProtocol);
			if (ptrKey->IsValue(L"Endpoint"))
				strHandler = ptrKey->GetValue(L"Endpoint").cast<string_t>();
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

		OTL::ObjectPtr<Omega::Remoting::IChannel> ptrChannel;
		if (m_mapRemoteChannels.find(channel.strEndpoint,ptrChannel))
			return ptrChannel.AddRef();
	}

	// Create a sink for the new endpoint
	channel.ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateInstancePtr();

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
		ptrChannel = channel.ptrRemoteChannel->client_init(this,ptrEndpoint,channel.strEndpoint,channel_id);
	}
	catch (...)
	{
		m_mapRemoteChannelIds.erase(channel_id);
		throw;
	}

	// Add to the maps
	err = m_mapRemoteChannels.insert(channel.strEndpoint,static_cast<Remoting::IChannel*>(ptrChannel));
	if (err != 0)
	{
		m_mapRemoteChannelIds.erase(channel_id);
		channel.ptrRemoteChannel->Close();
		OMEGA_THROW(err);
	}

	return ptrChannel.AddRef();
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
			LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
			pE->Release();
		}

		guard.acquire();
	}
}

OOServer::MessageHandler::io_result::type User::Manager::route_off(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no)
{
	try
	{
		ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel;
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

			RemoteChannelEntry channel_entry;
			if (!m_mapRemoteChannelIds.find(dest_channel_id & 0xFFF00000,channel_entry))
				return MessageHandler::route_off(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);

			ptrRemoteChannel = channel_entry.ptrRemoteChannel;
		}

		// Send it on...
		ptrRemoteChannel->send_away(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);

		return OOServer::MessageHandler::io_result::success;
	}
	catch (ITimeoutException* pE)
	{
		pE->Release();
		return OOServer::MessageHandler::io_result::timedout;
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		return OOServer::MessageHandler::io_result::channel_closed;
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls",pE->GetDescription().c_wstr()));
		pE->Release();
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
	channel.ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateInstancePtr();

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
		channel.ptrRemoteChannel->server_init(this,pSink,message_oid,channel_id);
	}
	catch (...)
	{
		m_mapRemoteChannelIds.erase(channel_id);
		throw;
	}

	return channel.ptrRemoteChannel.AddRef();
}

void User::Manager::local_channel_closed(OOBase::Stack<uint32_t,OOBase::LocalAllocator>& channels)
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
