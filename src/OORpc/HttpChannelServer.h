///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OORpc, the Omega Online RPC library.
//
// OORpc is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OORpc is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OORpc.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OORPC_HTTP_CHANNEL_H_INCLUDED_
#define OORPC_HTTP_CHANNEL_H_INCLUDED_

#include <OOCore/Http.h>
#include "./HttpMsg.h"

namespace Rpc
{
	// {AEA785BC-47B2-451b-ACFF-61C1DEE1AD25}
	OMEGA_EXPORT_OID(OID_HttpChannelServer);

	class HttpServerSink :
		public OTL::ObjectBase,
		public Omega::Remoting::IChannelSink
	{
	public:
		HttpServerSink();
		virtual ~HttpServerSink();

		void init();
		void handle_request(const Omega::string_t& strResource, Omega::IO::IStream* pRequest, Omega::Net::Http::Server::IResponse* pResponse);

		BEGIN_INTERFACE_MAP(HttpServerSink)
			INTERFACE_ENTRY(Omega::Remoting::IChannelSink)
		END_INTERFACE_MAP()

	private:
		struct Msg
		{
			Omega::Remoting::MethodAttributes_t  attribs;
			OTL::ObjectPtr<IHttpMsg>             ptrMsg;
			ACE_Time_Value                       deadline;
		};

		ACE_Thread_Mutex                              m_busy_lock;
		ACE_Message_Queue_Ex<Msg,ACE_MT_SYNCH>        m_msg_queue;
		OTL::ObjectPtr<Omega::IO::IStream>            m_ptrResponse;
		OTL::ObjectPtr<Omega::Remoting::IChannelSink> m_ptrSink;
		bool                                          m_bFirst;

		void Send_i();
		
	// IChannelSink members
	public:
		void Send(Omega::Remoting::MethodAttributes_t attribs, Omega::Remoting::IMessage* pMsg, Omega::uint32_t timeout);
		void Close();
	};

	class HttpChannelServer :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<HttpChannelServer,&OID_HttpChannelServer,0,Omega::Activation::InProcess>,
		public Omega::Net::Http::Server::IRequestHandler
	{
	public:
		HttpChannelServer()
		{ }

		virtual ~HttpChannelServer()
		{ }

		BEGIN_INTERFACE_MAP(HttpChannelServer)
			INTERFACE_ENTRY(Omega::Net::Http::Server::IRequestHandler)
		END_INTERFACE_MAP()

	private:
		HttpChannelServer(const HttpChannelServer&) { }
		HttpChannelServer& operator = (const HttpChannelServer&) { return *this; }

		ACE_RW_Thread_Mutex m_lock;
		Omega::string_t     m_strAbsURI;

		std::map<Omega::guid_t,OTL::ObjectPtr<OTL::ObjectImpl<HttpServerSink> > > m_mapSinks;

		void CreateNewConnection(Omega::Net::Http::Server::IResponse* pResponse);
		
	// IRequestHandler members
	public:
		void Open(const Omega::string_t& strAbsURI);
		void ProcessRequest(Omega::Net::Http::Server::IRequest* pRequest, Omega::Net::Http::Server::IResponse* pResponse);
	};
}

#endif // OORPC_HTTP_CHANNEL_H_INCLUDED_
