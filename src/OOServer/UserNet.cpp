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
		OMEGA_THROW(L"IEndpoint::Open returned null sink");

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

ObjectPtr<ObjectImpl<User::Channel> > User::RemoteChannel::create_channel(Omega::uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i = m_mapChannels.find(channel_id);
	if (i != m_mapChannels.end())
		ptrChannel = i->second;
	else
	{
		ptrChannel = ObjectImpl<User::Channel>::CreateInstancePtr();
		ptrChannel->init(m_pManager,m_channel_id | channel_id,Remoting::RemoteMachine,m_message_oid);
		
		m_mapChannels.insert(std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::value_type(channel_id,ptrChannel));
	}

	return ptrChannel;
}

ObjectPtr<Remoting::IObjectManager> User::RemoteChannel::create_object_manager(Omega::uint32_t channel_id)
{
	ObjectPtr<ObjectImpl<Channel> > ptrChannel = create_channel(channel_id);

	return ptrChannel->GetObjectManager();
}

void User::RemoteChannel::send_away(const OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no)
{
	// Make sure we have the source in the map...
	if (src_channel_id != 0)
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);
	
		std::map<Omega::uint32_t,Omega::uint32_t>::iterator i = m_mapChannelIds.find(src_channel_id);
		if (i != m_mapChannelIds.end())
			src_channel_id = i->second;
		else
		{
			Omega::uint32_t channel_id = 0;
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			} while (!channel_id && m_mapChannelIds.find(channel_id) != m_mapChannelIds.end());

			m_mapChannelIds.insert(std::map<Omega::uint32_t,Omega::uint32_t>::value_type(src_channel_id,channel_id));
			m_mapChannelIds.insert(std::map<Omega::uint32_t,Omega::uint32_t>::value_type(channel_id,src_channel_id));

			// Add our channel id to the source
			src_channel_id = channel_id;
		}
	}

	// Trim high bits off destination
	dest_channel_id &= 0x000FFFFF;

	ObjectPtr<Remoting::IMessage> ptrPayload;

	// Custom handling of system messages
	if (attribs & Root::Message_t::system_message)
	{
		if (flags == Root::Message_t::Request)
		{
			if ((attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
			{
				// Do nothing
			}
			else
				OMEGA_THROW(L"Invalid system message");
		}
		else if (flags == Root::Message_t::Response)
		{
			if ((attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
			{
				// Unpack the channel_id
				OOBase::CDRStream input(msg);
				Omega::uint32_t channel_id;
				input.read(channel_id);
				if (input.last_error() != 0)
					OMEGA_THROW(input.last_error());

				// Create a new message of the right format...
				if (m_message_oid == guid_t::Null())
					ptrPayload.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
				else
					ptrPayload = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::InProcess);

				// Write the channel id
				ptrPayload->WriteValue(L"channel_id",channel_id);
			}
			else
				OMEGA_THROW(L"Invalid system message");
		}
		else
			OMEGA_THROW(L"Invalid system message");
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
				throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

			ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrInput);
		}
	}
	
	send_away_i(ptrPayload,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
}

void User::RemoteChannel::send_away_i(Remoting::IMessage* pPayload, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no)
{
	// Create a new message of the right format...
	ObjectPtr<Remoting::IMessage> ptrMessage;
	if (m_message_oid == guid_t::Null())
		ptrMessage.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
	else
		ptrMessage = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::InProcess);

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
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

	ptrMarshaller->MarshalInterface(L"payload",ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);

	try
	{
		ptrMessage->WriteStructEnd();

		uint32_t timeout = 0;
		if (deadline != OOBase::timeval_t::MaxTime)
		{
			OOBase::timeval_t now = OOBase::gettimeofday();
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
		LOG_ERROR(("IException thrown: %ls - %ls",pE->GetDescription().c_str(),pE->GetSource().c_str()));
		pE->Release();
	}
	catch (...)
	{
		LOG_ERROR(("Unknown exception thrown"));
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
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",ptrMsg);
		
	// Check timeout
	uint32_t timeout = 0;
	if (deadline != OOBase::timeval_t::MaxTime)
	{
		OOBase::timeval_t now = OOBase::gettimeofday();
		if (deadline <= now)
			return;
		
		timeout = (deadline - now).msec();
	}

	ObjectPtr<Remoting::IMessage> ptrResult;
	ptrResult.Attach(ptrOM->Invoke(ptrPayload,timeout));

	if (!(ex_attribs & TypeInfo::Asynchronous))
	{
		if (deadline != OOBase::timeval_t::MaxTime)
		{
			if (deadline <= OOBase::gettimeofday())
				return;
		}

		// Send it back...
		send_away_i(ptrResult,0,src_channel_id,deadline,Root::Message_t::synchronous,src_thread_id,dest_thread_id,Root::Message_t::Response,seq_no);
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
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshaller),OMEGA_SOURCE_INFO);

	// Unmarshal payload
	ObjectPtr<Remoting::IMessage> ptrPayload = ptrMarshaller.UnmarshalInterface<Remoting::IMessage>(L"payload",pMsg);

	pMsg->ReadStructEnd();
	
	if (!dest_channel_id)
	{
		if (ex_attribs & Root::Message_t::system_message)
		{
			if (flags == Root::Message_t::Request)
			{
				ObjectPtr<Remoting::IMessage> ptrResult;
				uint32_t out_attribs = 0;

				if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_close)
				{
					uint32_t channel_id = ptrPayload->ReadValue(L"channel_id").cast<uint32_t>();

					m_pManager->channel_closed(channel_id | m_channel_id,0);

					out_attribs = Root::Message_t::asynchronous;
				}
				else if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
				{
					// Create a new message of the right format...
					if (m_message_oid == guid_t::Null())
						ptrResult.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
					else
						ptrResult = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::InProcess);
					
					// Send back the src_channel_id
					ptrResult->WriteValue(L"channel_id",src_channel_id | m_channel_id);

					out_attribs = Root::Message_t::synchronous | Root::Message_t::channel_reflect;
				}
				else
					OMEGA_THROW(L"Invalid system message");

				if (!(out_attribs & TypeInfo::Asynchronous))
				{
					// Send it back...
					send_away_i(ptrResult,dest_channel_id,src_channel_id,deadline,out_attribs,src_thread_id,dest_thread_id,Root::Message_t::Response,seq_no);
				}
			}
			else
				OMEGA_THROW(L"Invalid system message");
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

			if (!m_pManager->call_async_function_i(&process_here,this,&output))
			{
				Release();

				ptrMarshaller->ReleaseMarshalData(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
				OMEGA_THROW(L"Failed to queue message");
			}
		}
	}
	else
	{
		ObjectPtr<ObjectImpl<OOCore::CDRMessage> > ptrOutput = ObjectImpl<OOCore::CDRMessage>::CreateInstancePtr();

		if (ex_attribs & Root::Message_t::system_message)
		{
			// Filter the system messages
			if (flags == Root::Message_t::Request)
			{
				if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
				{
					// Pass on.. there is no payload to filter
				}
				else
					OMEGA_THROW(L"Invalid system message");
			}
			else if (flags == Root::Message_t::Response)
			{
				if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
				{
					// Unpack the channel_id
					uint32_t ch = ptrPayload->ReadValue(L"channel_id").cast<uint32_t>();
					
					// Repack in the right format
					ptrOutput->WriteValue(L"channel_id",ch);
				}
				else
					OMEGA_THROW(L"Invalid system message");
			}
			else
				OMEGA_THROW(L"Invalid system message");
		}
		else
		{
			// Marshal the message onto the CDR message
			ptrMarshaller->MarshalInterface(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
		}

		// Translate channel ids
		OOBase::Guard<OOBase::Mutex> guard(m_lock);
	
		std::map<Omega::uint32_t,Omega::uint32_t>::iterator i = m_mapChannelIds.find(dest_channel_id);
		if (i != m_mapChannelIds.end())
			dest_channel_id = i->second;
		else
		{
			Omega::uint32_t channel_id = 0;
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			} while (!channel_id && m_mapChannelIds.find(channel_id) != m_mapChannelIds.end());

			m_mapChannelIds.insert(std::map<Omega::uint32_t,Omega::uint32_t>::value_type(dest_channel_id,channel_id));
			m_mapChannelIds.insert(std::map<Omega::uint32_t,Omega::uint32_t>::value_type(channel_id,dest_channel_id));

			// Add our channel id to the source
			dest_channel_id = channel_id;
		}

		guard.release();
		
		// Trim high bits off destination
		src_channel_id |= m_channel_id;
	
		// Forward through the network...
		Root::MessageHandler::io_result::type res = m_pManager->forward_message(src_channel_id,dest_channel_id,deadline,ex_attribs,dest_thread_id,src_thread_id,flags,seq_no,*ptrOutput->GetCDRStream());
		if (res != Root::MessageHandler::io_result::success)
		{
			if (!(ex_attribs & Root::Message_t::system_message))
				ptrMarshaller->ReleaseMarshalData(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);

			if (res == Root::MessageHandler::io_result::timedout)
				throw Omega::ITimeoutException::Create();
			else if (res == Root::MessageHandler::io_result::channel_closed)
				throw Omega::Remoting::IChannelClosedException::Create();
			else
				OMEGA_THROW(L"Internal server exception");
		}
	}
}

void User::RemoteChannel::channel_closed(uint32_t channel_id)
{
	// Do this async
	OOBase::CDRStream output;
	output.write(channel_id);
	if (output.last_error() == 0)
	{
		AddRef();
		if (!m_pManager->call_async_function_i(&do_channel_closed,this,&output))
			Release();
	}
}

void User::RemoteChannel::do_channel_closed(void* pParam, OOBase::CDRStream& input)
{
	User::RemoteChannel* pThis = (User::RemoteChannel*)pParam;

	try
	{
		uint32_t channel_id = 0;
		input.read(channel_id);
		
		if (input.last_error() != 0)
			OMEGA_THROW(input.last_error());
		
		pThis->do_channel_closed_i(channel_id);
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls - %ls",pE->GetDescription().c_str(),pE->GetSource().c_str()));
		pE->Release();
	}
	catch (...)
	{
		LOG_ERROR(("Unknown exception thrown"));
	}

	pThis->Release();
}

void User::RemoteChannel::do_channel_closed_i(uint32_t channel_id)
{
	OOBase::Guard<OOBase::Mutex> guard(m_lock);

	std::map<Omega::uint32_t,Omega::uint32_t>::iterator i=m_mapChannelIds.find(channel_id);
	if (i != m_mapChannelIds.end())
	{
		// Create a new message of the right format...
		ObjectPtr<Remoting::IMessage> ptrMsg;
		if (m_message_oid == guid_t::Null())
			ptrMsg.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::CDRMessage>::CreateInstance()));
		else
			ptrMsg = ObjectPtr<Remoting::IMessage>(m_message_oid,Activation::InProcess);
		
		// Send back the src_channel_id
		ptrMsg->WriteValue(L"channel_id",i->second);

		guard.release();

		// Send a sys message
		send_away_i(ptrMsg,0,0,OOBase::timeval_t::MaxTime,Root::Message_t::asynchronous | Root::Message_t::channel_close,0,0,Root::Message_t::Request,0);
	}
}

void User::RemoteChannel::Close()
{
	OTL::ObjectPtr<Omega::Remoting::IChannelSink> ptrUpstream;
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		for (std::map<Omega::uint32_t,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();++i)
		{
			i->second->disconnect();
		}
		m_mapChannels.clear();

		if (m_ptrUpstream)
		{
			ptrUpstream = m_ptrUpstream;
			m_ptrUpstream.Release();

			// Tell the manager that channels have closed
			for (std::map<Omega::uint32_t,Omega::uint32_t>::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
			{
				if (i->first >= m_channel_id)
					break;

				m_pManager->channel_closed(i->first | m_channel_id,0);
			}

			m_pManager->channel_closed(m_channel_id,0);
		}
	}

	if (ptrUpstream)
		ptrUpstream->Close();
}

Remoting::IChannel* User::Manager::open_remote_channel(const string_t& strEndpoint)
{
	return USER_MANAGER::instance()->open_remote_channel_i(strEndpoint);
}

Remoting::IChannel* User::Manager::open_remote_channel_i(const string_t& strEndpoint)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified");
	
	string_t strProtocol = strEndpoint.Left(pos).ToLower();

	// Look up handler in registry
	string_t strHandler;
	ObjectPtr<Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"Endpoint"))
			strHandler = ptrKey->GetStringValue(L"Endpoint");
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Registry::IKey>(L"\\System");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"Endpoint"))
				strHandler = ptrKey->GetStringValue(L"Endpoint");
		}
	}

	guid_t oid = guid_t::Null();
	if (!strHandler.IsEmpty())
	{
		if (!guid_t::FromString(strHandler,oid))
			oid = Activation::NameToOid(strHandler);
	}

	if (oid == guid_t::Null())
		OMEGA_THROW(L"No handler for protocol " + strProtocol);

	// Create the factory
	ObjectPtr<Remoting::IEndpoint> ptrEndpoint(oid);

	// Check for duplicates
	string_t strCanon = ptrEndpoint->Canonicalise(strEndpoint);
	{
		void* TICKET_100;

		OOBase::ReadGuard<OOBase::RWMutex> guard(m_remote_lock);
		
		std::map<string_t,ObjectPtr<Remoting::IChannel> >::iterator i=m_mapRemoteChannels.find(strCanon);
		if (i != m_mapRemoteChannels.end())
			return i->second.AddRef();
	}

	// Create a sink for the new endpoint
	ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateInstancePtr();

	// Lock from here on...
	OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

	// Create a new unique, upstream channel...
	uint32_t channel_id = 0;
	try
	{
		while (!channel_id || m_mapRemoteChannelIds.find(channel_id) != m_mapRemoteChannelIds.end())
		{
			// Skip anything in the root bits
			if (m_nNextRemoteChannel & m_root_channel)
				m_nNextRemoteChannel = 0;

			channel_id = ((++m_nNextRemoteChannel << 20) & ~m_root_channel);
		}

		// Init the sink
		ObjectPtr<ObjectImpl<Channel> > ptrChannel = ptrRemoteChannel->client_init(this,ptrEndpoint,strCanon,channel_id);

		// Add to the maps
		RemoteChannelEntry channel;
		channel.ptrRemoteChannel = ptrRemoteChannel;
		channel.strEndpoint = strCanon;

		m_mapRemoteChannelIds.insert(std::map<uint32_t,RemoteChannelEntry>::value_type(channel_id,channel));
		m_mapRemoteChannels.insert(std::map<string_t,ObjectPtr<Remoting::IChannel> >::value_type(strCanon,static_cast<Remoting::IChannel*>(ptrChannel)));

		return ptrChannel.AddRef();
	}
	catch (...)
	{
		// Clean out the maps
		m_mapRemoteChannelIds.erase(channel_id);
		m_mapRemoteChannels.erase(strCanon);

		ptrRemoteChannel->Close();

		throw;
	}
}

void User::Manager::close_all_remotes()
{
	// Make a locked copy of the maps and close them
	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

		std::map<uint32_t,RemoteChannelEntry> channels(m_mapRemoteChannelIds);
			
		guard.release();

		for (std::map<uint32_t,RemoteChannelEntry>::iterator i = channels.begin();i!=channels.end();++i)
			i->second.ptrRemoteChannel->Close();
	}
	catch (std::exception& e)
	{
		LOG_ERROR(("std::exception thrown %s",e.what()));
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls - %ls",pE->GetDescription().c_str(),pE->GetSource().c_str()));
		pE->Release();
	}
	catch (...)
	{
		LOG_ERROR(("Unrecognised exception thrown"));
	}

	// Now spin, waiting for all the channels to close...
	OOBase::timeval_t wait(30);
	OOBase::Countdown countdown(&wait);
	while (wait != OOBase::timeval_t::Zero)
	{
		OOBase::ReadGuard<OOBase::RWMutex> guard(m_remote_lock);

		if (m_mapRemoteChannelIds.empty())
			break;

		guard.release();

		OOBase::sleep(OOBase::timeval_t(0,50000));

		countdown.update();
	}

	assert(m_mapRemoteChannelIds.empty());
}

Root::MessageHandler::io_result::type User::Manager::route_off(OOBase::CDRStream& msg, Omega::uint32_t src_channel_id, Omega::uint32_t dest_channel_id, const OOBase::timeval_t& deadline, Omega::uint32_t attribs, Omega::uint16_t dest_thread_id, Omega::uint16_t src_thread_id, Omega::uint16_t flags, Omega::uint32_t seq_no)
{
	try
	{
		ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel;
		{
			OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

			std::map<uint32_t,RemoteChannelEntry>::iterator i = m_mapRemoteChannelIds.find(dest_channel_id & 0xFFF00000);
			if (i == m_mapRemoteChannelIds.end())
				return MessageHandler::route_off(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);

			ptrRemoteChannel = i->second.ptrRemoteChannel;
		}

		// Send it on...
		ptrRemoteChannel->send_away(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);

		return Root::MessageHandler::io_result::success;
	}
	catch (ITimeoutException* pE)
	{
		pE->Release();
		return Root::MessageHandler::io_result::timedout;
	}
	catch (Remoting::IChannelClosedException* pE)
	{
		pE->Release();
		return Root::MessageHandler::io_result::channel_closed;
	}
	catch (IException* pE)
	{
		LOG_ERROR(("IException thrown: %ls - %ls",pE->GetDescription().c_str(),pE->GetSource().c_str()));
		pE->Release();
		return Root::MessageHandler::io_result::failed;
	}
}

Remoting::IChannelSink* User::Manager::open_server_sink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return USER_MANAGER::instance()->open_server_sink_i(message_oid,pSink);
}

Remoting::IChannelSink* User::Manager::open_server_sink_i(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	// Create a sink for the new endpoint
	ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateInstancePtr();

	// Lock from here on...
	OOBase::Guard<OOBase::RWMutex> guard(m_remote_lock);

	// Create a new unique, upstream channel...
	uint32_t channel_id = 0;
	try
	{
		while (!channel_id || m_mapRemoteChannelIds.find(channel_id) != m_mapRemoteChannelIds.end())
		{
			// Skip anything in the root bits
			if (m_nNextRemoteChannel & m_root_channel)
				m_nNextRemoteChannel = 0;

			channel_id = ((++m_nNextRemoteChannel << 20) & ~m_root_channel);
		}

		// Init the sink
		ptrRemoteChannel->server_init(this,pSink,message_oid,channel_id);

		// Add to the maps
		RemoteChannelEntry channel;
		channel.ptrRemoteChannel = ptrRemoteChannel;

		m_mapRemoteChannelIds.insert(std::map<uint32_t,RemoteChannelEntry>::value_type(channel_id,channel));

		return ptrRemoteChannel.AddRef();
	}
	catch (...)
	{
		// Clean out the maps
		m_mapRemoteChannelIds.erase(channel_id);

		ptrRemoteChannel->Close();

		throw;
	}
}

void User::Manager::local_channel_closed(Omega::uint32_t channel_id)
{
	// Local end has closed
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_remote_lock);

	for (std::map<uint32_t,RemoteChannelEntry>::iterator i=m_mapRemoteChannelIds.begin();i!=m_mapRemoteChannelIds.end();++i)
	{
		if ((channel_id & 0xFFF00000) != i->first)
			i->second.ptrRemoteChannel->channel_closed(channel_id);
	}
}
