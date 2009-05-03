///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOBASE_WIN32_SHM_SOCKET_H_INCLUDED_
#define OOBASE_WIN32_SHM_SOCKET_H_INCLUDED_

#include "Socket.h"
#include "SmartPtr.h"

#if defined(_WIN32)

namespace OOBase
{
	namespace Win32
	{
		class ShmSocketImpl
		{
		public:
			bool init_server(const std::string& strName, OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout, SECURITY_ATTRIBUTES* psa);
			bool init_client(OOBase::LocalSocket* via, int* perr, const OOBase::timeval_t* timeout);
			
		protected:
			ShmSocketImpl() {}
			virtual ~ShmSocketImpl();

			OOBase::Win32::SmartHandle   m_hMapping;
			bool                         m_bServer;
			OOBase::Win32::SmartHandle   m_hPipe;
			OOBase::Win32::SmartHandle   m_close_event;

			struct OV : public OVERLAPPED
			{
				char buf;
			};
			OOBase::SmartPtr<OV> m_ptrOv;
			
			struct Fifo
			{
				struct SharedInfo
				{
					static const size_t buffer_size = 8192 - (2*sizeof(size_t));

					volatile size_t m_read_pos;
					volatile size_t m_write_pos;
					char            m_data[buffer_size];
				};

				Fifo() : m_shared(0)
				{}

				SharedInfo*                m_shared;
				OOBase::Win32::SmartHandle m_read_event;
				OOBase::Win32::SmartHandle m_write_event;

				size_t recv_i(char*& data, size_t& len, int* perr);
				int send_i(const char*& data, size_t& len);

				inline bool is_full();
				inline bool is_empty();
			};

			Fifo m_fifos[2];
			bool create_fifos(int* perr, const char* name);
			int bind_socket(OOBase::LocalSocket* via);

			void close();
		};
	}
}

#endif // defined(_WIN32)

#endif // OOBASE_WIN32_SHM_SOCKET_H_INCLUDED_
