///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2013 Rick Taylor
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

#include "OOServer_Registry.h"
#include "RegistryRootConn.h"
#include "RegistryManager.h"

Registry::RootConnection::RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock)
{
}

Registry::RootConnection::~RootConnection()
{
	// Tell the manager that we have closed
	m_pManager->on_root_closed();
}

bool Registry::RootConnection::start()
{
	addref();

	int err = OOBase::CDRIO::recv_with_header_sync(size_t(256),m_socket,this,&RootConnection::on_message_start);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Registry::RootConnection::on_message_start(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::StackAllocator<256> allocator;
		OOBase::LocalString strDb(allocator);
		stream.read_string(strDb);

		if (stream.last_error())
			LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			LOG_DEBUG(("OPEN DB: %s",strDb.c_str()));

			// Tell the manager to create a new connection
			int ret_err = 0;//m_pManager->new_connection(sid,pipe);

			err = stream.reset();
			if (err)
				LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				size_t mark = stream.buffer()->mark_wr_ptr();

				stream.write(size_t(0));
				stream.write(ret_err);

				stream.replace(stream.buffer()->length(),mark);
				if (stream.last_error())
					LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					LOG_DEBUG(("Sending response %lu bytes",stream.buffer()->length()));

					err = m_socket->send(this,NULL,stream.buffer());
					if (err)
						LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
					else
						recv_next();
				}
			}
		}
	}

	release();
}

bool Registry::RootConnection::recv_next()
{
#if defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = new (std::nothrow) OOBase::Buffer(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text()),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync(size_t(128),m_socket,this,&RootConnection::on_message_posix,ctl_buffer);
#elif defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync(size_t(128),m_socket,this,&RootConnection::on_message_win32);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

#if defined(HAVE_UNISTD_H)

void Registry::RootConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		// Start another receive...
		recv_next();

		OOBase::POSIX::SmartFD passed_fd;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);
		if (msg)
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				if (msg->cmsg_len != CMSG_LEN(sizeof(int)))
				{
					LOG_ERROR(("Root pipe control data looks wrong"));
					err = EINVAL;
				}
				else
					passed_fd = *reinterpret_cast<int*>(CMSG_DATA(msg));
			}
			else
			{
				LOG_ERROR(("Root pipe control data has weird stuff in it"));
				err = EINVAL;
			}

			if (CMSG_NXTHDR(&msgh,msg) != NULL)
			{
				LOG_ERROR(("Root pipe control data has extra stuff in it"));
				passed_fd.close();
				err = EINVAL;
			}
		}

		if (!err)
		{
			// Read and cache any root parameters
			OOBase::StackAllocator<256> allocator;
			OOBase::LocalString p1(allocator),p2(allocator);
			stream.read_string(p1);
			stream.read_string(p2);

			uid_t uid;
			stream.read(uid);
			gid_t gid;
			stream.read(gid);

			if (stream.last_error())
				LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				// Tell the manager to create a new connection
				int ret_err = m_pManager->new_connection(passed_fd,uid,gid);
				if (!ret_err)
					passed_fd.detach();

				err = stream.reset();
				if (err)
					LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					size_t mark = stream.buffer()->mark_wr_ptr();

					stream.write(size_t(0));
					stream.write(ret_err);
					if (!ret_err)
					{
						stream.write_string(p1);
						stream.write_string(p2);
					}

					stream.replace(stream.buffer()->length(),mark);
					if (stream.last_error())
						LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
					else
					{
						err = m_socket->send(this,NULL,stream.buffer());
						if (err)
							LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
					}
				}
			}
		}
	}

	release();
}

#elif defined(_WIN32)

void Registry::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		// Start another receive...
		recv_next();

		// Read and cache any root parameters
		OOBase::StackAllocator<256> allocator;
		OOBase::LocalString p1(allocator),p2(allocator),sid(allocator),pipe(allocator);
		stream.read_string(p1);
		stream.read_string(p2);
		stream.read_string(sid);

		if (stream.last_error())
			LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			// Tell the manager to create a new connection
			int ret_err = m_pManager->new_connection(sid,pipe);

			err = stream.reset();
			if (err)
				LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				size_t mark = stream.buffer()->mark_wr_ptr();

				stream.write(size_t(0));
				stream.write(ret_err);
				if (!ret_err)
				{
					stream.write_string(p1);
					stream.write_string(p2);
					stream.write_string(pipe);
				}

				stream.replace(stream.buffer()->length(),mark);
				if (stream.last_error())
					LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					err = m_socket->send(this,NULL,stream.buffer());
					if (err)
						LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
				}
			}
		}
	}

	release();
}

#endif
