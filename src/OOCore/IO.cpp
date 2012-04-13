///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "../../include/Omega/IO.h"

using namespace Omega;
using namespace OTL;

namespace
{
	class BufferInputStream :
			public ObjectBase,
			public IO::IInputStream
	{
	public:
		BufferInputStream() :
				m_bOwn(false),
				m_len(0),
				m_pos(0),
				m_data(0)
		{}

		virtual ~BufferInputStream()
		{
			if (m_bOwn)
				System::Free(m_data);
		}

		void init(size_t lenBytes, const byte_t* data, bool bCopy)
		{
			m_len = lenBytes;
			m_bOwn = bCopy;

			if (!m_bOwn)
				m_data = const_cast<byte_t*>(data);
			else if (m_len)
			{
				m_data = static_cast<byte_t*>(System::Allocate(m_len));
				memcpy(m_data,data,m_len);
			}
		}

		BEGIN_INTERFACE_MAP(BufferInputStream)
			INTERFACE_ENTRY(IO::IInputStream)
		END_INTERFACE_MAP()

	private:
		Threading::Mutex m_lock;
		bool             m_bOwn;
		size_t           m_len;
		size_t           m_pos;
		byte_t*          m_data;

	public:
		uint32_t ReadBytes(uint32_t lenBytes, byte_t* data)
		{
			Threading::Guard guard(m_lock);

			size_t r = lenBytes;
			if (m_pos + r > m_len)
				r = m_len - m_pos;

			memcpy(data,m_data + m_pos,r);
			m_pos += r;

			return static_cast<uint32_t>(r);
		}
	};

	class FileInputStream :
			public ObjectBase,
			public IO::IInputStream
	{
	public:
		BEGIN_INTERFACE_MAP(FileInputStream)
			INTERFACE_ENTRY(IO::IInputStream)
		END_INTERFACE_MAP()

#if defined(_WIN32)
	public:
		FileInputStream() : m_hFile(INVALID_HANDLE_VALUE)
		{}

		virtual ~FileInputStream()
		{
			if (m_hFile != INVALID_HANDLE_VALUE)
				::CloseHandle(m_hFile);
		}

		void init(HANDLE hFile)
		{
			m_hFile = hFile;
		}

		uint32_t ReadBytes(uint32_t lenBytes, byte_t* data)
		{
			DWORD dwRead = 0;
			if (!::ReadFile(m_hFile,data,lenBytes,&dwRead,NULL))
				throw ISystemException::Create(GetLastError());

			return static_cast<uint32_t>(dwRead);
		}

	private:
		HANDLE m_hFile;
#else
	public:
		FileInputStream() : m_fd(-1)
		{}

		virtual ~FileInputStream()
		{
			if (m_fd != -1)
				::close(m_fd);
		}

		void init(int fd)
		{
			m_fd = fd;
		}

		uint32_t ReadBytes(uint32_t lenBytes, byte_t* data)
		{
			do
			{
				ssize_t r = ::read(m_fd,data,lenBytes);
				if (r != -1)
					return static_cast<uint32_t>(r);
			}
			while (errno == EINTR);

			throw ISystemException::Create(errno);
		}

	private:
		int m_fd;
#endif
	};
}

OMEGA_DEFINE_EXPORTED_FUNCTION(IO::IInputStream*,OOCore_IO_CreateInputStream_1,3,((in),size_t,lenBytes,(in)(size_is(lenBytes)),const byte_t*,data,(in),bool_t,bCopy))
{
	ObjectPtr<ObjectImpl<BufferInputStream> > ptrRet = ObjectImpl<BufferInputStream>::CreateInstance();

	ptrRet->init(lenBytes,data,bCopy);

	return ptrRet.Detach();
}

#if defined(_WIN32)
OMEGA_DEFINE_EXPORTED_FUNCTION(IO::IInputStream*,OOCore_IO_CreateInputStream_2,1,((in),HANDLE,hFile))
{
	ObjectPtr<ObjectImpl<FileInputStream> > ptrRet = ObjectImpl<FileInputStream>::CreateInstance();

	ptrRet->init(hFile);

	return ptrRet.Detach();
}
#else
OMEGA_DEFINE_EXPORTED_FUNCTION(IO::IInputStream*,OOCore_IO_CreateInputStream_2,1,((in),int,fd))
{
	ObjectPtr<ObjectImpl<FileInputStream> > ptrRet = ObjectImpl<FileInputStream>::CreateInstance();

	ptrRet->init(fd);

	return ptrRet.Detach();
}
#endif
