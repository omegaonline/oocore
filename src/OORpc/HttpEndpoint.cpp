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

#include <OOCore/Http.h>

#include "./HttpEndpoint.h"
#include "./HttpChannelServer.h"
#include "./HttpMsg.h"

using namespace Omega;
using namespace OTL;

OMEGA_DEFINE_OID(Rpc,OID_HttpEndpoint,"{62CF3425-D60F-4f3e-920B-6CAFDD8AAA77}");

namespace Rpc
{
	class SendNotify :
		public ObjectBase,
		public Net::Http::IRequestNotify
	{
	public:
		void init(const string_t& strSessionId, const string_t& strEndpoint);
		void send(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pMsg, const ACE_Time_Value& deadline);
		void close();

		BEGIN_INTERFACE_MAP(SendNotify)
			INTERFACE_ENTRY(Net::Http::IRequestNotify)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                    m_busy_lock;
		string_t                            m_strSessionId;
		string_t                            m_strEndpoint;
		ObjectPtr<Net::Http::IRequest>      m_ptrReq;

		struct Msg
		{
			Remoting::MethodAttributes_t  attribs;
			ObjectPtr<IHttpMsg>           ptrMsg;
			ACE_Time_Value                deadline;
		};
		ACE_Message_Queue_Ex<Msg,ACE_MT_SYNCH> m_msg_queue;

		void Send_i();

	// Net::Http::IRequestNotify members
	public:
		void OnResponseStart(uint16_t nCode, const string_t& strMsg);
		void OnResponseDataAvailable();
		void OnResponseComplete();
		void OnError(IException* pE);
	};

	class RecvNotify :
		public ObjectBase,
		public Net::Http::IRequestNotify
	{
	public:
		void init(const string_t& strSessionId, const string_t& strEndpoint, Remoting::IChannelSink* pSink);
		void close();

		BEGIN_INTERFACE_MAP(RecvNotify)
			INTERFACE_ENTRY(Net::Http::IRequestNotify)
		END_INTERFACE_MAP()

	private:
		string_t                            m_strSessionId;
		string_t                            m_strEndpoint;
		ObjectPtr<Net::Http::IRequest>      m_ptrReq;
		ObjectPtr<Remoting::IChannelSink>   m_ptrSink;

		void open();

	// Net::Http::IRequestNotify members
	public:
		void OnResponseStart(uint16_t nCode, const string_t& strMsg);
		void OnResponseDataAvailable();
		void OnResponseComplete();
		void OnError(IException* pE);
	};

	class HttpChannelSink :
		public ObjectBase,
		public Remoting::IChannelSink
	{
	public:
		HttpChannelSink();
		virtual ~HttpChannelSink();

		void init(const string_t& strSessionId, const string_t& strEndpoint, Remoting::IChannelSink* pSink);

		BEGIN_INTERFACE_MAP(HttpChannelSink)
			INTERFACE_ENTRY(Remoting::IChannelSink)
		END_INTERFACE_MAP()

	private:
		ObjectPtr<ObjectImpl<SendNotify> > m_ptrSend;
		ObjectPtr<ObjectImpl<RecvNotify> > m_ptrRecv;

	// IChannelSink members
	public:
		void Send(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pMsg, uint32_t timeout);
		void Close();
	};
}

void Rpc::SendNotify::init(const string_t& strSessionId, const string_t& strEndpoint)
{
	m_strSessionId = strSessionId;
	m_strEndpoint = strEndpoint;

	m_ptrReq = ObjectPtr<Net::Http::IRequest>(Net::Http::OID_StdHttpRequest);
}

void Rpc::SendNotify::send(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pMsg, const ACE_Time_Value& deadline)
{
	try
	{
		// Enqueue
		Msg* msg = 0;
		OMEGA_NEW(msg,Msg);

		msg->attribs = attribs;
		msg->ptrMsg = pMsg;
		if (!msg->ptrMsg)
			OMEGA_THROW(L"Invalid message type!");

		msg->deadline = deadline;

		if (m_msg_queue.enqueue_tail(msg,&msg->deadline) == -1)
			OMEGA_THROW(ACE_OS::last_error());

		// If we aren't busy, start a new POST
		if (m_busy_lock.tryacquire() == 0)
			Send_i();
	}
	catch (IException* pE)
	{
		pE->Release();
		close();
	}
}

void Rpc::SendNotify::close()
{
	m_ptrReq.Release();
}

void Rpc::SendNotify::Send_i()
{
	if (!m_ptrReq)
		return;

	// Start a POST
	m_ptrReq->Open(L"POST",m_strEndpoint + L"up?oosid=" + m_strSessionId,this);
	m_ptrReq->SetRequestHeader(L"Transfer-Encoding",L"chunked");
	m_ptrReq->SetRequestHeader(L"Cache-Control",L"no-cache");

	std::ostringstream osChunk;
	osChunk.flags(std::ios::hex);

	size_t count = m_msg_queue.message_count();
	uint64_t cbTotal = 0;
	bool bFirst = true;
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

			if (bFirst)
				bFirst = false;
			else
				osContent << " ,";

			osContent << "{ \"attribs\": " << msg->attribs << ", ";
			osContent << strContent << " }";

			if (!osContent.good())
				OMEGA_THROW(ACE_OS::last_error());

			strContent = osContent.str();

			osChunk << strContent.length() << "\r\n" << strContent << "\r\n";

			cbTotal += strContent.length();
		}

		delete msg;

		// Set an upper limit to a single send - lets be nice
		if (cbTotal > 30000)
			break;
	}

	// Append a final 0 chunk
	osChunk << "0\r\n\r\n";
	if (!osChunk.good())
		OMEGA_THROW(ACE_OS::last_error());

	std::string strSend = osChunk.str();

	// Send the whole damn thing!
	m_ptrReq->Send((uint32_t)strSend.length(),(const byte_t*)strSend.c_str());
}

void Rpc::SendNotify::OnResponseStart(uint16_t nCode, const string_t&)
{
	// Check for error condition
	if (nCode != 200)
		close();
}

void Rpc::SendNotify::OnResponseDataAvailable()
{
	// NOP
}

void Rpc::SendNotify::OnResponseComplete()
{
	try
	{
		// If there is nothing else pending
		if (m_msg_queue.is_empty())
		{
			// We are no longer busy
			m_busy_lock.release();
			return;
		}

		// Send the next load...
		Send_i();
	}
	catch (IException* pE)
	{
		pE->Release();
		close();
	}
}

void Rpc::SendNotify::OnError(IException*)
{
	close();
}

void Rpc::RecvNotify::init(const string_t& strSessionId, const string_t& strEndpoint, Remoting::IChannelSink* pSink)
{
	m_strSessionId = strSessionId;
	m_strEndpoint = strEndpoint;
	m_ptrSink = pSink;

	open();
}

void Rpc::RecvNotify::open()
{
	m_ptrReq = ObjectPtr<Net::Http::IRequest>(Net::Http::OID_StdHttpRequest);
	m_ptrReq->Open(L"POST",m_strEndpoint + L"down?oosid=" + m_strSessionId,this);
	m_ptrReq->SetRequestHeader(L"Content-Length",L"0");
	m_ptrReq->SetRequestHeader(L"Cache-Control",L"no-cache");
	m_ptrReq->Send();
}

void Rpc::RecvNotify::close()
{
	if (m_ptrSink)
		m_ptrSink->Close();

	m_ptrReq.Release();
}

void Rpc::RecvNotify::OnResponseStart(uint16_t nCode, const string_t&)
{
	// Check for error condition
	if (nCode != 200)
		close();
}

void Rpc::RecvNotify::OnResponseDataAvailable()
{
	if (!m_ptrReq)
		return;

	// It's one or more commands...
	try
	{
		// Build a message block of the contents
		ObjectPtr<IO::IStream> ptrStream;
		ptrStream.Attach(m_ptrReq->ResponseStream());

		size_t ps = ACE_OS::getpagesize();
		ACE_Message_Block* mb;
		OMEGA_NEW(mb,ACE_Message_Block(ps));

		for (;;)
		{
			uint64_t cbSize = ps;
			if (mb->space() < cbSize)
			{
				if (mb->size(mb->size() + ps) != 0)
					OMEGA_THROW(ACE_OS::last_error());
			}

			ptrStream->ReadBytes(cbSize,(byte_t*)mb->wr_ptr());
			if (cbSize == 0)
				break;

			mb->wr_ptr(static_cast<size_t>(cbSize));
		}

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
				m_ptrSink->Send(attribs,ptrInput,0);

			ptrInput->ReadStructEnd(0);
		}
	}
	catch (IException* pE)
	{
		pE->Release();
		close();
	}
}

void Rpc::RecvNotify::OnResponseComplete()
{
	// Try to reopen
	try
	{
		open();
	}
	catch (IException* pE)
	{
		pE->Release();
		close();
	}
}

void Rpc::RecvNotify::OnError(IException*)
{
	// This is fatal
	close();
}

Rpc::HttpChannelSink::HttpChannelSink()
{
}

Rpc::HttpChannelSink::~HttpChannelSink()
{
	if (m_ptrSend)
		m_ptrSend->close();

	if (m_ptrRecv)
		m_ptrRecv->close();
}

void Rpc::HttpChannelSink::init(const string_t& strSessionId, const string_t& strEndpoint, Remoting::IChannelSink* pSink)
{
	// Open the upstream and downstream channels
	m_ptrSend = ObjectImpl<SendNotify>::CreateInstancePtr();
	m_ptrSend->init(strSessionId,strEndpoint);

	m_ptrRecv = ObjectImpl<RecvNotify>::CreateInstancePtr();
	m_ptrRecv->init(strSessionId,strEndpoint,pSink);
}

void Rpc::HttpChannelSink::Send(Remoting::MethodAttributes_t attribs, Remoting::IMessage* pMsg, uint32_t timeout)
{
	if (!m_ptrSend)
		OMEGA_THROW(ENOTCONN);

	ACE_Time_Value deadline = ACE_Time_Value::max_time;
	if (timeout)
		deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout / 1000,(timeout % 1000) * 1000);

	m_ptrSend->send(attribs,pMsg,deadline);
}

void Rpc::HttpChannelSink::Close()
{
	if (m_ptrSend)
	{
		m_ptrSend->close();
		m_ptrSend.Release();
	}

	if (m_ptrRecv)
	{
		m_ptrRecv->close();
		m_ptrRecv.Release();
	}
}

string_t Rpc::HttpEndpoint::Canonicalise(const string_t& strEndpoint)
{
	// Parse the request out to its bits...
	string_t strScheme, strHost, strPort, strUserName, strPassword, strResource, strQuery;
	Net::Http::SplitURL(strEndpoint,strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);

	// Don't use usernames
	string_t strCanon = strScheme + L"://" + strHost;
	if (strPort.IsEmpty())
	{
		if (strScheme == L"http")
			strPort = L"80";
		else if (strScheme == L"https")
			strPort = L"443";
	}
	strCanon += L":" + strPort;

	// Default to OID_HttpChannelServer
	if (strResource == L"/")
		strResource += OID_HttpChannelServer.ToString().Mid(1,36).ToLower() + L"/";

	// Reappend resource and query
	strCanon += strResource;
	if (!strQuery.IsEmpty())
		strCanon += L"?" + strQuery;

	return strCanon;
}

Omega::guid_t Rpc::HttpEndpoint::MessageOid()
{
	return OID_HttpOutputMsg;
}

Remoting::IChannelSink* Rpc::HttpEndpoint::Open(const string_t& strEndpoint, Remoting::IChannelSink* pSink)
{
	// Parse the request out to its bits... We could do something with the parts here...
	/*string_t strScheme, strHost, strPort, strUserName, strPassword, strResource, strQuery;
	Net::Http::SplitURL(strEndpoint,strScheme,strHost,strPort,strUserName,strPassword,strResource,strQuery);*/

	// Create the Std Http Requests
	ObjectPtr<Net::Http::IRequest> ptrReq = ObjectPtr<Net::Http::IRequest>(Net::Http::OID_StdHttpRequest);

	// Do a POST to start, this will allow us to get the oosid guid
	ptrReq->Open(L"POST",strEndpoint + L"?init=true");
	ptrReq->SetRequestHeader(L"Content-Length",L"0");
	ptrReq->SetRequestHeader(L"Cache-Control",L"no-cache");
	ptrReq->Send();

	if (ptrReq->Status() != 200)
		OMEGA_THROW(L"Remote http server response: " + ptrReq->StatusText());

	// Unpack the response and set up everything else
	char szBuf[41] = {0};
	uint32_t cbBytes = 40;
	ptrReq->ResponseBody(cbBytes,(byte_t*)szBuf);

	string_t strSessionId(szBuf,true);
	if (strSessionId.IsEmpty() || guid_t::FromString(L"{" + strSessionId + L"}") == guid_t::Null())
		OMEGA_THROW(L"Expected to receive a session id from the server");

	// Create a new channel
	ObjectPtr<ObjectImpl<HttpChannelSink> > ptrSink = ObjectImpl<HttpChannelSink>::CreateInstancePtr();
	ptrSink->init(strSessionId,strEndpoint,pSink);

	return ptrSink.AddRef();
}

void Rpc::HttpEndpoint::install(Omega::bool_t bInstall, const Omega::string_t& strSubsts)
{
	// Register ourselves
	string_t strXML =
		L"<?xml version=\"1.0\"?>"
		L"<root xmlns=\"http://www.omegaonline.org.uk/schemas/registry.xsd\">"
			L"<key name=\"\\Networking\\Protocols\">"
				L"<key name=\"http\" uninstall=\"Remove\">"
					L"<value name=\"Endpoint\">Omega.Rpc.HttpEndpoint</value>"
				L"</key>"
				L"<key name=\"https\" uninstall=\"Remove\">"
					L"<value name=\"Endpoint\">Omega.Rpc.HttpEndpoint</value>"
				L"</key>"
			L"</key>"
		L"</root>";

	Omega::Registry::AddXML(strXML,bInstall,strSubsts);
}
