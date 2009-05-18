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

#ifndef OOSERVER_NETTCP_H_INCLUDED_
#define OOSERVER_NETTCP_H_INCLUDED_

namespace User
{
	// {4924E463-06A4-483b-9DAD-8BFD83ADCBFC}
	OMEGA_EXPORT_OID(OID_TcpProtocolHandler);

	class TcpHandler
	{
	public:
		TcpHandler();

		void AddRef()
		{
			++m_refcount;
		}

		void Release()
		{
			if (--m_refcount == 0)
				delete this;
		}

		void AsyncRead(Omega::uint32_t stream_id, ACE_Message_Block* mb, size_t len);
		void AsyncWrite(Omega::uint32_t stream_id, ACE_Message_Block* mb);
		void AsyncClose(Omega::uint32_t stream_id);
		Omega::string_t GetRemoteEndpoint(Omega::uint32_t stream_id);
		Omega::string_t GetLocalEndpoint(Omega::uint32_t stream_id);

		void Start();
		void Stop();

		Omega::Net::IConnectedStream* OpenStream(const Omega::string_t& strEndpoint, Omega::IO::IAsyncStreamNotify* pNotify);

	private:
#if defined(ACE_WIN32)
		class TcpAsync : public ACE_Service_Handler
#else
		class TcpAsync : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
#endif
		{
		public:
			TcpAsync() :
				m_pHandler(0), m_stream_id(0), m_refcount(1)
			{}

			TcpAsync(TcpHandler* pHandler) :
				m_pHandler(pHandler), m_stream_id(0), m_refcount(1)
			{
				m_pHandler->AddRef();
			}

			void release()
			{
				if (--m_refcount == 0)
					delete this;
			}

			bool read(ACE_Message_Block& mb, size_t len);
			bool write(ACE_Message_Block& mb);
			Omega::string_t local_endpoint();
			Omega::string_t remote_endpoint();

#if defined(ACE_WIN32)
			void act(const void* pv);
			void open(ACE_HANDLE new_handle, ACE_Message_Block&);
			void close();
#else
			int open(void* act = 0);
#endif

		private:
			TcpHandler*             m_pHandler;
			Omega::uint32_t        m_stream_id;

#if defined(ACE_WIN32)
			ACE_SOCK_Stream         m_stream;
			ACE_Asynch_Read_Stream  m_reader;
			ACE_Asynch_Write_Stream m_writer;

			void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
			void handle_write_stream(const ACE_Asynch_Write_Stream::Result& result);
#endif

			// Control our lifetime...
			OOBase::AtomicInt<unsigned long> m_refcount;

			virtual ~TcpAsync()
			{
				if (m_pHandler)
					m_pHandler->Release();
			}

			static void error_thunk(void* pParam, ACE_InputCDR& input);
			static void open_stream_thunk(void* pParam, ACE_InputCDR& input);
			static void handle_read_stream_thunk(void* pParam, ACE_InputCDR& input);
			static void handle_write_stream_thunk(void* pParam, ACE_InputCDR& input);
		};
		friend class TcpAsync;

#if defined(ACE_WIN32)
		class AsyncConnector : public ACE_Asynch_Connector<TcpAsync>
#else
		class AsyncConnector : public ACE_Connector<TcpAsync,ACE_SOCK_CONNECTOR>
#endif
		{
		public:
			TcpHandler* m_pHandler;

			bool connect(const ACE_INET_Addr& addr, Omega::uint32_t stream_id);

		protected:
#if defined(ACE_WIN32)
			void handle_connect(const ACE_Asynch_Connect::Result& result);
			TcpAsync* make_handler();
#else
			int make_svc_handler(TcpAsync*& handler);
#endif

		private:
			static void call_error(void* pParam, ACE_InputCDR& input);
		};
		AsyncConnector m_connector;

		struct AsyncEntry
		{
			OTL::ObjectPtr<Omega::IO::IAsyncStreamNotify> ptrNotify;
			TcpAsync*                                     pAsync;
		};
		OOBase::RWMutex                      m_lock;
		Omega::uint32_t                      m_nNextStream;
		std::map<Omega::uint32_t,AsyncEntry> m_mapAsyncs;

		// Control our lifetime...
		OOBase::AtomicInt<unsigned long>     m_refcount;

		virtual ~TcpHandler() {};
		TcpHandler(const TcpHandler&) {};
		TcpHandler& operator = (const TcpHandler&) { return *this; };

		void OnAsyncOpen(Omega::uint32_t stream_id);
		void OnAsyncError(Omega::uint32_t stream_id, int err);
		void OnAsyncRead(Omega::uint32_t stream_id, const ACE_Message_Block* mb);
		void OnAsyncWrite(Omega::uint32_t stream_id, size_t len);

		bool add_async(Omega::uint32_t stream_id, TcpAsync* pAsync);
		void remove_async(Omega::uint32_t stream_id);
	};

	class TcpProtocolHandler :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactorySingleton<TcpProtocolHandler,&OID_TcpProtocolHandler,Omega::Activation::InProcess>,
		public Omega::System::IService,
		public Omega::Net::IProtocolHandler
	{
	public:
		TcpProtocolHandler();
		virtual ~TcpProtocolHandler();

		BEGIN_INTERFACE_MAP(TcpProtocolHandler)
			INTERFACE_ENTRY(Omega::System::IService)
			INTERFACE_ENTRY(Omega::Net::IProtocolHandler)
		END_INTERFACE_MAP()

	private:
		TcpProtocolHandler(const TcpProtocolHandler&) {};
		TcpProtocolHandler& operator = (const TcpProtocolHandler&) { return *this; }

		OOBase::Mutex m_lock;
		TcpHandler*   m_pHandler;

	// IService members
	public:
		void Start();
		void Stop();

	// IProtocolHandler members
	public:
		Omega::Net::IConnectedStream* OpenStream(const Omega::string_t& strEndpoint, Omega::IO::IAsyncStreamNotify* pNotify);
	};
}

#endif // OOSERVER_NETTCP_H_INCLUDED_