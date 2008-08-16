///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
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

#include "OORpc.h"

#include "./HttpChannelServer.h"
#include "./HttpMsg.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(Rpc,OID_HttpChannelServer,"{AEA785BC-47B2-451b-ACFF-61C1DEE1AD25}");

Rpc::HttpServerSink::HttpServerSink()
{
}

Rpc::HttpServerSink::~HttpServerSink()
{
}

void Rpc::HttpServerSink::init()
{
	m_ptrSink.Attach(Remoting::IChannelSink::OpenServerSink(Rpc::OID_HttpOutputMsg,this));
}

void Rpc::HttpServerSink::handle_request(const Omega::string_t& strResource, Omega::IO::IStream* pRequest, Net::Http::Server::IResponse* pResponse)
{
	if (strResource == L"/up")
	{
		// It's one or more commands...

		// Build a message block of the contents
		size_t ps = ACE_OS::getpagesize();
		ACE_Message_Block* mb;
		OMEGA_NEW(mb,ACE_Message_Block(ps));

		for (;;)
		{
			if (mb->space() < ps)
			{
				if (mb->size(mb->size() + ps) != 0)
					OMEGA_THROW(ACE_OS::last_error());
			}

			uint64_t cbSize = ps;
			pRequest->ReadBytes(cbSize,(byte_t*)mb->wr_ptr());
			if (cbSize == 0)
				break;

			mb->wr_ptr(static_cast<size_t>(cbSize));
		}

		// Respond with an OK...
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Cache-Control",L"no-cache");
		pResponse->Send(200,L"OK");

		// Wrap the message block
		ObjectPtr<ObjectImpl<HttpInputMsg> > ptrInput = ObjectImpl<HttpInputMsg>::CreateInstancePtr();
		ptrInput->init(mb);
		mb->release();

		// Loop unpacking requests
		while (ptrInput->more_exists())
		{
			ptrInput->ReadStructStart(0,0);

			uint16_t attribs = 0;
			ptrInput->ReadUInt16s(L"attribs",1,&attribs);

			// Send on the command
			if (m_ptrSink)
			{
				try
				{
					m_ptrSink->Send(attribs,ptrInput,0);
				}
				catch (IException* pE)
				{
					pE->Release();
					Close();
				}
				catch (...)
				{
					Close();
				}
			}

			ptrInput->ReadStructEnd(0);
		}
	}
	else if (strResource == L"/down")
	{
		// It's a request for events...
		pResponse->SetResponseHeader(L"Transfer-Encoding",L"chunked");
		pResponse->SetResponseHeader(L"Cache-Control",L"no-cache");
		pResponse->SetResponseHeader(L"Content-Type",L"text/plain; charset=utf-8");

		m_ptrResponse.Attach(pResponse->Send(200,L"OK"));

		// If we aren't busy, start responding
		if (m_busy_lock.acquire() == 0)
		{
			m_bFirst = true;

			Send_i();

			m_busy_lock.release();
		}
	}
	else
	{
		// It's duff!
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(404,L"Not Found");
	}
}

void Rpc::HttpServerSink::Send(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pMsg, uint32_t timeout)
{
	// Forward the message...

	// Enqueue
	Msg* msg = 0;
	OMEGA_NEW(msg,Msg);

	msg->attribs = attribs;
	msg->ptrMsg = pMsg;
	if (!msg->ptrMsg)
		OMEGA_THROW(L"Invalid message type!");

	if (timeout == 0)
		msg->deadline = ACE_Time_Value::max_time;
	else
		msg->deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout / 1000,(timeout % 1000) * 1000);

	if (m_msg_queue.enqueue_tail(msg,&msg->deadline) == -1)
		OMEGA_THROW(ACE_OS::last_error());

	// If we aren't busy, start responding
	if (m_ptrResponse && m_busy_lock.tryacquire() == 0)
	{
		Send_i();

		m_busy_lock.release();
	}
}

void Rpc::HttpServerSink::Send_i()
{
	try
	{
		size_t count = m_msg_queue.message_count();
		while (count > 0)
		{
			// Dequeue the next message
			Msg* msg = 0;
			count = m_msg_queue.dequeue_head(msg);
			if (count == (size_t)-1)
				OMEGA_THROW(ACE_OS::last_error());

			// Check deadline
			if (msg->deadline > ACE_OS::gettimeofday())
			{
				std::string strContent;
				msg->ptrMsg->GetContent(&strContent);

				std::ostringstream osContent;

				if (m_bFirst)
					m_bFirst = false;
				else
					osContent << ",";

				osContent << "{ \"attribs\": " << msg->attribs << ", ";
				osContent << strContent << " }";

				if (!osContent.good())
					OMEGA_THROW(ACE_OS::last_error());

				strContent = osContent.str();

				std::ostringstream osChunk;
				osChunk.flags(std::ios::hex);

				osChunk << static_cast<uint32_t>(strContent.length()) << "\r\n" << strContent << "\r\n";
				strContent = osChunk.str();

				// Send the whole damn thing!
				m_ptrResponse->WriteBytes(static_cast<uint32_t>(strContent.length()),(const byte_t*)strContent.c_str());
			}

			delete msg;
		}
	}
	catch (IException* pE)
	{
		pE->Release();

		m_ptrResponse.Release();
	}
}

void Rpc::HttpServerSink::Close()
{
	m_ptrResponse.Release();

	if (m_ptrSink)
	{
		OTL::ObjectPtr<Omega::Remoting::IChannelSink> ptrSink = m_ptrSink;
		m_ptrSink.Release();

		ptrSink->Close();
	}
}

void Rpc::HttpChannelServer::Open(const string_t& strAbsURI)
{
	m_strAbsURI = strAbsURI;
}

void Rpc::HttpChannelServer::ProcessRequest(Net::Http::Server::IRequest* pRequest, Net::Http::Server::IResponse* pResponse)
{
	string_t strRes = pRequest->Resource();
	string_t strQuery;

	// Find the query string
	size_t pos = strRes.Find(L'?');
	if (pos != string_t::npos)
	{
		strQuery = strRes.Mid(pos+1);
		strRes = strRes.Left(pos);
	}

	string_t strMethod = pRequest->Method();
	if (strMethod != L"POST")
	{
		pResponse->SetResponseHeader(L"Allow",L"POST");
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(405,L"Method Not Allowed");
		return;
	}

	// See if we are init
	if (strQuery == L"init=true")
	{
		// Yes! Create a new cookie!
		CreateNewConnection(pResponse);
		return;
	}

	// See if we have a cookie
	pos = strQuery.Find(L"oosid=");
	if (pos == string_t::npos)
	{
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(404,L"Not Found");
		return;
	}

	strQuery = strQuery.Mid(pos+6);
	pos = strQuery.Find(L';');
	if (pos == string_t::npos)
		pos = strQuery.Find(L'&');

	if (pos != string_t::npos)
		strQuery = strQuery.Left(pos).TrimRight();

	if (strQuery.IsEmpty())
	{
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(404,L"Not Found");
		return;
	}

	// Find the Sink object and call it...
	guid_t oosid = guid_t::FromString(L"{" + strQuery + L"}");
	if (oosid == guid_t::Null())
	{
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(404,L"Not Found");
		return;
	}

	ObjectPtr<IO::IStream> ptrStream;
	ptrStream.Attach(pRequest->RequestStream());

	// We must have a payload
	if (!ptrStream)
	{
		pResponse->SetResponseHeader(L"Content-Length",L"0");
		pResponse->SetResponseHeader(L"Connection",L"close");
		pResponse->Send(404,L"Not Found");
		return;
	}

	ObjectPtr<ObjectImpl<HttpServerSink> > ptrSink;
	try
	{
		OORPC_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<guid_t,ObjectPtr<ObjectImpl<HttpServerSink> > >::iterator i = m_mapSinks.find(oosid);
		if (i == m_mapSinks.end())
		{
			pResponse->SetResponseHeader(L"Content-Length",L"0");
			pResponse->SetResponseHeader(L"Connection",L"close");
			pResponse->Send(404,L"Not Found");
			return;
		}

		ptrSink = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	ptrSink->handle_request(strRes,ptrStream,pResponse);
}

void Rpc::HttpChannelServer::CreateNewConnection(Net::Http::Server::IResponse* pResponse)
{
	string_t strScheme, strHost, strPort, strUserName, strPassword, strResource, strQuery;
	Net::Http::SplitURL(m_strAbsURI + L"/",strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);

	// Create a new session id
	guid_t oosid = guid_t::Create();

	// Create a new Sink object
	ObjectPtr<ObjectImpl<HttpServerSink> > ptrSink = ObjectImpl<HttpServerSink>::CreateInstancePtr();
	ptrSink->init();

	// Add it to the map
	try
	{
		OORPC_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<guid_t,ObjectPtr<ObjectImpl<HttpServerSink> > >::iterator,bool> p = m_mapSinks.insert(std::map<guid_t,ObjectPtr<ObjectImpl<HttpServerSink> > >::value_type(oosid,ptrSink));
		if (!p.second)
			OMEGA_THROW(L"Guid clash!");
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	// Reply OK with the oosid
	std::string strResponse = oosid.ToString().ToUTF8();
	strResponse = strResponse.substr(1,strResponse.size()-2);

	pResponse->SetResponseHeader(L"Content-Length",string_t::Format(L"%lu",strResponse.length()));
	pResponse->SetResponseHeader(L"Content-Type",L"text/plain; charset=utf-8");
	pResponse->SetResponseHeader(L"Cache-Control",L"no-cache");

	ObjectPtr<IO::IStream> ptrStream;
	ptrStream.Attach(pResponse->Send(200,L"OK"));

	ptrStream->WriteBytes(strResponse.length(),(const byte_t*)strResponse.c_str());
}
