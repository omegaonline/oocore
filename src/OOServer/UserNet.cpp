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

#include "./OOServer_User.h"
#include "./UserManager.h"
#include "./MessageConnection.h"

using namespace Omega;
using namespace OTL;

User::RemoteChannel::RemoteChannel() :
	m_pManager(0),
	m_channel_id(0),
	m_nNextChannelId(0)
{
}

Remoting::IObjectManager* User::RemoteChannel::client_init(Manager* pManager, Remoting::IEndpoint* pEndpoint, const string_t& strEndpoint, uint32_t channel_id)
{
	m_pManager = pManager;
	m_channel_id = channel_id;

	// Open the remote endpoint and attach ourselves as the sink...
	m_ptrUpstream.Attach(pEndpoint->Open(strEndpoint,this));
	if (!m_ptrUpstream)
		OMEGA_THROW(L"IEndpoint::Open returned null sink!");

	m_message_oid = pEndpoint->MessageOid();

	// Create a local channel around the new id (the remote channel will do the actual routing)
	return create_object_manager(0).AddRef();
}

void User::RemoteChannel::server_init(Manager* pManager, Remoting::IChannelSink* pSink, const guid_t& message_oid, uint32_t channel_id)
{
	m_pManager = pManager;
	m_channel_id = channel_id;
	m_ptrUpstream = pSink;
	m_message_oid = message_oid;

	// Create a local channel around the new id (the remote channel will do the actual routing)
	create_object_manager(0);
}

ObjectPtr<Remoting::IObjectManager> User::RemoteChannel::create_object_manager(ACE_CDR::ULong channel_id)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	ObjectPtr<ObjectImpl<Channel> > ptrChannel;
	std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i = m_mapChannels.find(channel_id);
	if (i != m_mapChannels.end())
		ptrChannel = i->second;
	else
	{
		ptrChannel = ObjectImpl<User::Channel>::CreateInstancePtr();
		ptrChannel->init(m_channel_id | channel_id,Remoting::RemoteMachine,m_message_oid);
		
		m_mapChannels.insert(std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::value_type(channel_id,ptrChannel));
	}

	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());
	return ptrOM;
}

void User::RemoteChannel::send_away(const ACE_InputCDR& msg, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no)
{
	// Make sure we have the source in the map...
	if (src_channel_id != 0)
	{
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);
	
		std::map<ACE_CDR::ULong,ACE_CDR::ULong>::iterator i = m_mapChannelIds.find(src_channel_id);
		if (i != m_mapChannelIds.end())
			src_channel_id = i->second;
		else
		{
			ACE_CDR::ULong channel_id = 0;
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			} while (!channel_id && m_mapChannelIds.find(channel_id) != m_mapChannelIds.end());

			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ACE_CDR::ULong>::value_type(src_channel_id,channel_id));
			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ACE_CDR::ULong>::value_type(channel_id,src_channel_id));

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
				OMEGA_THROW(L"Invalid system message!");
		}
		else if (flags == Root::Message_t::Response)
		{
			if ((attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
			{
				// Unpack the channel_id
				ACE_InputCDR input(msg);
				ACE_CDR::ULong channel_id;
				input >> channel_id;
				if (!input.good_bit())
					OMEGA_THROW(ACE_OS::last_error());

				// Create a new message of the right format...
				if (m_message_oid == guid_t::Null())
					ptrPayload.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::OutputCDR>::CreateInstance()));
				else
					ptrPayload.CreateInstance(m_message_oid,Activation::InProcess);

				// Write the channel id
				ptrPayload->WriteUInt32s(L"channel_id",1,&channel_id);
			}
			else
				OMEGA_THROW(L"Invalid system message!");
		}
		else
			OMEGA_THROW(L"Invalid system message!");
	}
	else
	{
		// Unmarshal the payload
		if (msg.length() > 0)
		{
			// Wrap the message block
			ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrInput = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
			ptrInput->init(msg);

			ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);
			
			IObject* pPayload = 0;
			ptrOM->UnmarshalInterface(L"payload",ptrInput,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
			ptrPayload.Attach(static_cast<Remoting::IMessage*>(pPayload));
		}
	}
	
	send_away_i(ptrPayload,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
}

void User::RemoteChannel::send_away_i(Remoting::IMessage* pPayload, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no)
{
	// Create a new message of the right format...
	ObjectPtr<Remoting::IMessage> ptrMessage;
	if (m_message_oid == guid_t::Null())
		ptrMessage.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::OutputCDR>::CreateInstance()));
	else
		ptrMessage.CreateInstance(m_message_oid,Activation::InProcess);

	// Write the mesage struct
	ptrMessage->WriteStructStart(L"message",L"$rpc_msg");
	ptrMessage->WriteUInt32s(L"src_channel_id",1,&src_channel_id);
	ptrMessage->WriteUInt32s(L"dest_channel_id",1,&dest_channel_id);
	
	// Fiddle with remote deadline value...
	uint64_t secs = 0;
	int32_t usecs = 0;
	if (deadline != ACE_Time_Value::max_time)
	{
		secs = deadline.sec();
		usecs = deadline.usec();
	}
	ptrMessage->WriteUInt64s(L"deadline_secs",1,&secs);
	ptrMessage->WriteInt32s(L"deadline_usecs",1,&usecs);
	ptrMessage->WriteUInt32s(L"attribs",1,&attribs);
	ptrMessage->WriteUInt16s(L"dest_thread_id",1,&dest_thread_id);
	ptrMessage->WriteUInt16s(L"src_thread_id",1,&src_thread_id);
	ptrMessage->WriteUInt16s(L"flags",1,&flags);
	ptrMessage->WriteUInt32s(L"seq_no",1,&seq_no);

	// Get the source channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);
	ptrOM->MarshalInterface(L"payload",ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);

	try
	{
		ptrMessage->WriteStructEnd(L"message");

		uint32_t timeout = 0;
		if (deadline != ACE_Time_Value::max_time)
		{
			ACE_Time_Value now = ACE_OS::gettimeofday();
			if (deadline <= now)
				OMEGA_THROW(ETIMEDOUT);

			timeout = (deadline - now).msec();
		}

		if (!m_ptrUpstream)
			OMEGA_THROW(ECONNRESET);

		m_ptrUpstream->Send((Remoting::MethodAttributes_t)(attribs & 0xFFFF),ptrMessage,timeout);
	}
	catch (...)
	{
		ptrMessage->ReadStructStart(L"message",L"$rpc_msg");
		ptrMessage->ReadUInt32s(L"src_channel_id",1,&src_channel_id);
		ptrMessage->ReadUInt32s(L"dest_channel_id",1,&dest_channel_id);
		uint64_t secs;
		int32_t usecs;
		ptrMessage->ReadUInt64s(L"deadline_secs",1,&secs);
		ptrMessage->ReadInt32s(L"deadline_usecs",1,&usecs);
		ptrMessage->ReadUInt32s(L"attribs",1,&attribs);
		ptrMessage->ReadUInt16s(L"dest_thread_id",1,&dest_thread_id);
		ptrMessage->ReadUInt16s(L"src_thread_id",1,&src_thread_id);
		ptrMessage->ReadUInt16s(L"flags",1,&flags);
		ptrMessage->ReadUInt32s(L"seq_no",1,&seq_no);
		ptrOM->ReleaseMarshalData(L"payload",ptrMessage,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
		throw;
	}
}

void User::RemoteChannel::process_here(void* pParams, ACE_InputCDR& input)
{
	RemoteChannel* pThis = static_cast<RemoteChannel*>(pParams);

	try
	{
		pThis->process_here_i(input);
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->Description().c_str(),pE->Source().c_str()));

		pE->Release();
	}
	catch (...)
	{
	}
}

void User::RemoteChannel::process_here_i(ACE_InputCDR& input)
{
	ObjectPtr<ObjectImpl<OOCore::InputCDR> > ptrMsg = ObjectImpl<OOCore::InputCDR>::CreateInstancePtr();
	ptrMsg->init(input);

	ACE_InputCDR* pInput = static_cast<ACE_InputCDR*>(ptrMsg->GetInputCDR());

	uint32_t src_channel_id;
	(*pInput) >> src_channel_id;
	uint64_t secs = 0;
	(*pInput) >> secs;
	int32_t usecs = 0;
	(*pInput) >> usecs;
	
	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	if (secs != 0 && usecs != 0)
		deadline = ACE_Time_Value((time_t)secs,usecs);

	uint32_t ex_attribs = 0;
	(*pInput) >> ex_attribs;
	uint16_t dest_thread_id;
	(*pInput) >> dest_thread_id;
	uint16_t src_thread_id;
	(*pInput) >> src_thread_id;
	uint32_t seq_no;
	(*pInput) >> seq_no;
	if (!pInput->good_bit())
		OMEGA_THROW(ACE_OS::last_error());

	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(src_channel_id);

	// Check timeout
	uint32_t timeout = 0;
	if (deadline != ACE_Time_Value::max_time)
	{
		ACE_Time_Value now = ACE_OS::gettimeofday();
		if (deadline <= now)
		{
			ACE_OS::last_error(ETIMEDOUT);
			return;
		}
		timeout = (deadline - now).msec();
	}

	IObject* pPayload = 0;
	ptrOM->UnmarshalInterface(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
	ObjectPtr<Remoting::IMessage> ptrPayload;
	ptrPayload.Attach(static_cast<Remoting::IMessage*>(pPayload));
		
	ObjectPtr<Remoting::IMessage> ptrResult;
	ptrResult.Attach(ptrOM->Invoke(ptrPayload,timeout));

	if (!(ex_attribs & Remoting::Asynchronous))
	{
		// Send it back...
		send_away_i(ptrResult,0,src_channel_id,deadline,Root::Message_t::synchronous,src_thread_id,dest_thread_id,Root::Message_t::Response,seq_no);
	}
}

void User::RemoteChannel::Send(Remoting::MethodAttributes_t, Remoting::IMessage* pMsg, uint32_t timeout)
{
	// This is a message from the other end...

	// Unpack parameters
	pMsg->ReadStructStart(L"message",L"$rpc_msg");
	uint32_t src_channel_id;
	pMsg->ReadUInt32s(L"src_channel_id",1,&src_channel_id);
	uint32_t dest_channel_id;
	pMsg->ReadUInt32s(L"dest_channel_id",1,&dest_channel_id);
	uint64_t secs = 0;
	pMsg->ReadUInt64s(L"deadline_secs",1,&secs);
	int32_t usecs = 0;
	pMsg->ReadInt32s(L"deadline_usecs",1,&usecs);
	uint32_t ex_attribs = 0;
	pMsg->ReadUInt32s(L"attribs",1,&ex_attribs);

	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	if (secs != 0 && usecs != 0)
		deadline = ACE_Time_Value((time_t)secs,usecs);

	if (timeout != 0)
	{
		ACE_Time_Value tdeadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout/1000,(timeout % 1000) * 1000);
		if (tdeadline < deadline)
			deadline = tdeadline;
	}

	// Fiddle with remote deadline value...

	uint16_t dest_thread_id;
	pMsg->ReadUInt16s(L"dest_thread_id",1,&dest_thread_id);
	uint16_t src_thread_id;
	pMsg->ReadUInt16s(L"src_thread_id",1,&src_thread_id);
	uint16_t flags;
	pMsg->ReadUInt16s(L"flags",1,&flags);
	uint32_t seq_no;
	pMsg->ReadUInt32s(L"seq_no",1,&seq_no);

	// Get the dest channel OM
	ObjectPtr<Remoting::IObjectManager> ptrOM = create_object_manager(dest_channel_id);

	IObject* pPayload = 0;
	ptrOM->UnmarshalInterface(L"payload",pMsg,OMEGA_GUIDOF(Remoting::IMessage),pPayload);
	ObjectPtr<Remoting::IMessage> ptrPayload;
	ptrPayload.Attach(static_cast<Remoting::IMessage*>(pPayload));

	pMsg->ReadStructEnd(L"message");
	
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
					uint32_t channel_id = 0;
					ptrPayload->ReadUInt32s(L"channel_id",1,&channel_id);

					m_pManager->channel_closed(channel_id | m_channel_id,0);

					out_attribs = Root::Message_t::asynchronous;
				}
				else if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
				{
					// Create a new message of the right format...
					if (m_message_oid == guid_t::Null())
						ptrResult.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::OutputCDR>::CreateInstance()));
					else
						ptrResult.CreateInstance(m_message_oid,Activation::InProcess);
					
					// Send back the src_channel_id
					uint32_t ch = src_channel_id | m_channel_id;

					ptrResult->WriteUInt32s(L"channel_id",1,&ch);

					out_attribs = Root::Message_t::synchronous | Root::Message_t::channel_reflect;
				}
				else
					OMEGA_THROW(L"Bad system message!");

				if (!(out_attribs & Remoting::Asynchronous))
				{
					// Send it back...
					send_away_i(ptrResult,dest_channel_id,src_channel_id,deadline,out_attribs,src_thread_id,dest_thread_id,Root::Message_t::Response,seq_no);
				}
			}
			else
				OMEGA_THROW(L"Bad system message!");
		}
		else
		{
			// Need to queue this as an async func...
			ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrMsg = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();
			ACE_OutputCDR* pOutput = static_cast<ACE_OutputCDR*>(ptrMsg->GetOutputCDR());

			(*pOutput) << src_channel_id;
			uint64_t secs = 0;
			int32_t usecs = 0;
			if (deadline != ACE_Time_Value::max_time)
			{
				secs = deadline.sec();
				usecs = deadline.usec();
			}
			(*pOutput) << secs;
			(*pOutput) << usecs;
			(*pOutput) << ex_attribs;
			(*pOutput) << dest_thread_id;
			(*pOutput) << src_thread_id;
			(*pOutput) << seq_no;
			if (!pOutput->good_bit())
				OMEGA_THROW(ACE_OS::last_error());

			ptrOM->MarshalInterface(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
			
			if (!m_pManager->call_async_function(process_here,this,static_cast<const ACE_Message_Block*>(ptrMsg->GetMessageBlock())))
			{
				ptrOM->ReleaseMarshalData(L"payload",ptrMsg,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
				OMEGA_THROW(L"Failed to queue message");
			}
		}
	}
	else
	{
		ObjectPtr<ObjectImpl<OOCore::OutputCDR> > ptrOutput = ObjectImpl<OOCore::OutputCDR>::CreateInstancePtr();

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
					OMEGA_THROW(L"Bad system message!");
			}
			else if (flags == Root::Message_t::Response)
			{
				if ((ex_attribs & Root::Message_t::system_message) == Root::Message_t::channel_reflect)
				{
					// Unpack the channel_id
					uint32_t ch = 0;
					ptrPayload->ReadUInt32s(L"channel_id",1,&ch);
					
					// Repack in the right format
					ptrOutput->WriteUInt32s(L"channel_id",1,&ch);
				}
				else
					OMEGA_THROW(L"Bad system message!");
			}
			else
				OMEGA_THROW(L"Bad system message!");
		}
		else
		{
			// Marshal the message onto the CDR message
			ptrOM->MarshalInterface(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
		}

		// Translate channel ids
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);
	
		std::map<ACE_CDR::ULong,ACE_CDR::ULong>::iterator i = m_mapChannelIds.find(dest_channel_id);
		if (i != m_mapChannelIds.end())
			dest_channel_id = i->second;
		else
		{
			ACE_CDR::ULong channel_id = 0;
			do
			{
				channel_id = ++m_nNextChannelId;
				if (channel_id & 0xFFF00000)
				{
					m_nNextChannelId = 0;
					channel_id = 0;
				}
			} while (!channel_id && m_mapChannelIds.find(channel_id) != m_mapChannelIds.end());

			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ACE_CDR::ULong>::value_type(dest_channel_id,channel_id));
			m_mapChannelIds.insert(std::map<ACE_CDR::ULong,ACE_CDR::ULong>::value_type(channel_id,dest_channel_id));

			// Add our channel id to the source
			dest_channel_id = channel_id;
		}

		guard.release();
		
		// Trim high bits off destination
		src_channel_id |= m_channel_id;
	
		// Forward through the network...
		const ACE_Message_Block* mb = static_cast<const ACE_Message_Block*>(ptrOutput->GetMessageBlock());
		if (!m_pManager->forward_message(src_channel_id,dest_channel_id,deadline,ex_attribs,dest_thread_id,src_thread_id,flags,seq_no,mb))
		{
			int err = ACE_OS::last_error();
			ptrOM->ReleaseMarshalData(L"payload",ptrOutput,OMEGA_GUIDOF(Remoting::IMessage),ptrPayload);
			OMEGA_THROW(err);
		}
	}
}

void User::RemoteChannel::channel_closed(uint32_t channel_id)
{
	// Do this async
	ACE_OutputCDR output;
	output << channel_id;
	
	if (output.good_bit())
		m_pManager->call_async_function(&do_channel_closed,this,output.begin());
}

void User::RemoteChannel::do_channel_closed(void* pParam, ACE_InputCDR& input)
{
	User::RemoteChannel* pThis = (User::RemoteChannel*)pParam;

	try
	{
		uint32_t channel_id = 0;
		input >> channel_id;
		
		if (!input.good_bit())
			OMEGA_THROW(ACE_OS::last_error());
		
		pThis->do_channel_closed_i(channel_id);
	}
	catch (IException* pE)
	{
		ACE_ERROR((LM_ERROR,ACE_TEXT("%W: Unhandled exception: %W\n"),pE->Description().c_str(),pE->Source().c_str()));

		pE->Release();
	}
	catch (...)
	{
	}
}

void User::RemoteChannel::do_channel_closed_i(uint32_t channel_id)
{
	OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

	std::map<ACE_CDR::ULong,ACE_CDR::ULong>::iterator i=m_mapChannelIds.find(channel_id);
	if (i != m_mapChannelIds.end())
	{
		// Create a new message of the right format...
		ObjectPtr<Remoting::IMessage> ptrMsg;
		if (m_message_oid == guid_t::Null())
			ptrMsg.Attach(static_cast<Remoting::IMessage*>(ObjectImpl<OOCore::OutputCDR>::CreateInstance()));
		else
			ptrMsg.CreateInstance(m_message_oid,Activation::InProcess);
		
		// Send back the src_channel_id
		ptrMsg->WriteUInt32s(L"channel_id",1,&i->second);

		guard.release();

		// Send a sys message
		send_away_i(ptrMsg,0,0,ACE_Time_Value::max_time,Root::Message_t::asynchronous | Root::Message_t::channel_close,0,0,Root::Message_t::Request,0);
	}
}

void User::RemoteChannel::Close()
{
	OTL::ObjectPtr<Omega::Remoting::IChannelSink> ptrUpstream;
	{
		OOSERVER_GUARD(ACE_Thread_Mutex,guard,m_lock);

		for (std::map<ACE_CDR::ULong,ObjectPtr<ObjectImpl<Channel> > >::iterator i=m_mapChannels.begin();i!=m_mapChannels.end();++i)
		{
			i->second->disconnect();
		}
		m_mapChannels.clear();

		if (m_ptrUpstream)
		{
			ptrUpstream = m_ptrUpstream;
			m_ptrUpstream.Release();

			// Tell the manager that channels have closed
			for (std::map<ACE_CDR::ULong,ACE_CDR::ULong>::iterator i=m_mapChannelIds.begin();i!=m_mapChannelIds.end();++i)
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

Remoting::IObjectManager* User::Manager::open_remote_channel(const string_t& strEndpoint)
{
	return USER_MANAGER::instance()->open_remote_channel_i(strEndpoint);
}

Remoting::IObjectManager* User::Manager::open_remote_channel_i(const string_t& strEndpoint)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndpoint.Left(pos).ToLower();

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
		ptrKey = ObjectPtr<Registry::IKey>(L"\\");
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
		oid = guid_t::FromString(strHandler);
		if (oid == guid_t::Null())
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

		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

		std::map<string_t,ObjectPtr<Remoting::IObjectManager> >::iterator i=m_mapRemoteChannels.find(strCanon);
		if (i != m_mapRemoteChannels.end())
			return i->second.AddRef();
	}

	// Create a sink for the new endpoint
	ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel = ObjectImpl<RemoteChannel>::CreateInstancePtr();

	// Lock from here on...
	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

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
		ObjectPtr<Remoting::IObjectManager> ptrOM;
		ptrOM.Attach(ptrRemoteChannel->client_init(this,ptrEndpoint,strCanon,channel_id));

		// Add to the maps
		RemoteChannelEntry channel;
		channel.ptrRemoteChannel = ptrRemoteChannel;
		channel.strEndpoint = strCanon;

		m_mapRemoteChannelIds.insert(std::map<uint32_t,RemoteChannelEntry>::value_type(channel_id,channel));
		m_mapRemoteChannels.insert(std::map<string_t,ObjectPtr<Remoting::IObjectManager> >::value_type(strCanon,ptrOM));

		return ptrOM.AddRef();
	}
	catch (std::exception& e)
	{
		// Clean out the maps
		m_mapRemoteChannelIds.erase(channel_id);
		m_mapRemoteChannels.erase(strCanon);

		ptrRemoteChannel->Close();

		OMEGA_THROW(e);
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
	try
	{
		std::map<uint32_t,RemoteChannelEntry> channels;

		// Make a locked copy of the maps and clear them
		{
			ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

			channels = m_mapRemoteChannelIds;
			m_mapRemoteChannels.clear();
			m_mapRemoteChannelIds.clear();
		}

		for (std::map<uint32_t,RemoteChannelEntry>::iterator i = channels.begin();i!=channels.end();++i)
			i->second.ptrRemoteChannel->Close();

		channels.clear();
	}
	catch (IException* pE)
	{
		pE->Release();
	}
	catch (...)
	{}
}

bool User::Manager::route_off(ACE_InputCDR& msg, ACE_CDR::ULong src_channel_id, ACE_CDR::ULong dest_channel_id, const ACE_Time_Value& deadline, ACE_CDR::ULong attribs, ACE_CDR::UShort dest_thread_id, ACE_CDR::UShort src_thread_id, ACE_CDR::UShort flags, ACE_CDR::ULong seq_no)
{
	try
	{
		ObjectPtr<ObjectImpl<RemoteChannel> > ptrRemoteChannel;
		{
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

			std::map<uint32_t,RemoteChannelEntry>::iterator i = m_mapRemoteChannelIds.find(dest_channel_id & 0xFFF00000);
			if (i == m_mapRemoteChannelIds.end())
				return MessageHandler::route_off(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);

			ptrRemoteChannel = i->second.ptrRemoteChannel;
		}

		// Send it on...
		ptrRemoteChannel->send_away(msg,src_channel_id,dest_channel_id,deadline,attribs,dest_thread_id,src_thread_id,flags,seq_no);
		return true;
	}
	catch (IException* pE)
	{
		pE->Release();
		return false;
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
	OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

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
	catch (std::exception& e)
	{
		// Clean out the maps
		m_mapRemoteChannelIds.erase(channel_id);

		ptrRemoteChannel->Close();

		OMEGA_THROW(e);
	}
	catch (...)
	{
		// Clean out the maps
		m_mapRemoteChannelIds.erase(channel_id);

		ptrRemoteChannel->Close();

		throw;
	}
}

void User::Manager::local_channel_closed(ACE_CDR::ULong channel_id)
{
	// Local end has closed
	OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_remote_lock);

	for (std::map<uint32_t,RemoteChannelEntry>::iterator i=m_mapRemoteChannelIds.begin();i!=m_mapRemoteChannelIds.end();++i)
	{
		if ((channel_id & 0xFFF00000) != i->first)
			i->second.ptrRemoteChannel->channel_closed(channel_id);
	}
}
