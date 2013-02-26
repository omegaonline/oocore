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
	m_pManager->quit();
}

bool Registry::RootConnection::start()
{
	addref();

	// Read start-up params
	int err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(256,m_socket,this,&RootConnection::on_start);
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to read root request: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

void Registry::RootConnection::on_start(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		Omega::uint16_t response_id = 0;
		stream.read(response_id);

		// Read DB name and root settings
		OOBase::StackAllocator<512> allocator;
		OOBase::LocalString strDb(allocator);
		stream.read_string(strDb);

		Omega::byte_t nThreads = 0;
		stream.read(nThreads);

		OOBase::Table<OOBase::LocalString,OOBase::LocalString,OOBase::AllocatorInstance> tabSettings(allocator);
		while (!err)
		{
			OOBase::LocalString k(allocator),v(allocator);
			if (!stream.read_string(k) || k.empty() || !stream.read_string(v))
				break;

			err = tabSettings.insert(k,v);
			if (err)
				LOG_ERROR(("Failed to insert setting into table: %s",OOBase::system_error_text(err)));
		}
		if (!err)
		{
			err = stream.last_error();
			if (err)
				LOG_ERROR(("Failed to read root request: %s",OOBase::system_error_text(err)));
		}

		int ret_err = 0;
		if (!err)
			ret_err = m_pManager->on_start(strDb,nThreads,tabSettings);

		if (!err)
		{
			err = stream.reset();
			if (err)
				LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				size_t mark = stream.buffer()->mark_wr_ptr();

				stream.write(Omega::uint16_t(0));
				stream.write(response_id);
				stream.write(static_cast<Omega::int32_t>(ret_err));

				stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
				if (stream.last_error())
					LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					addref();

					err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
					if (err)
					{
						LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));

						release();
					}
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
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_message_posix,ctl_buffer);
#elif defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_message_win32);
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
	if (stream.buffer()->length() == 0)
		LOG_ERROR(("Root pipe disconnected"));
	else if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::POSIX::SmartFD passed_fd;

		// Read struct cmsg
		struct msghdr msgh = {0};
		msgh.msg_control = const_cast<char*>(ctl_buffer->rd_ptr());
		msgh.msg_controllen = ctl_buffer->length();

		for (struct cmsghdr* msg = CMSG_FIRSTHDR(&msgh);msg;msg = CMSG_NXTHDR(&msgh,msg))
		{
			if (msg->cmsg_level == SOL_SOCKET && msg->cmsg_type == SCM_RIGHTS)
			{
				int* fds = reinterpret_cast<int*>(CMSG_DATA(msg));
				size_t fd_count = (msg->cmsg_len - CMSG_LEN(0))/sizeof(int);
				size_t fd_start = 0;
				if (fd_count > 0)
				{
					err = OOBase::POSIX::set_close_on_exec(fds[0],true);
					if (!err)
					{
						passed_fd = fds[0];
						fd_start = 1;
					}
				}

				for (size_t i=fd_start;i<fd_count;++i)
					OOBase::POSIX::close(fds[i]);
			}
			else
			{
				LOG_ERROR(("Root pipe control data has weird stuff in it"));
				err = EINVAL;
			}
		}

		if (!passed_fd.is_valid())
		{
			err = EINVAL;
			LOG_ERROR(("Root pipe control data invalid handle"));
		}

		if (!err)
			on_message(stream,passed_fd);
	}

	release();
}

void Registry::RootConnection::new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
{
	Omega::uint16_t response_id;
	stream.read(response_id);
	uid_t uid;
	stream.read(uid);

	printf("NewConenction: uid %u, response_id %u\n",uid,response_id);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		// Attach to the pipe
		int ret_err = 0;
		OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->attach(passed_fd,ret_err);
		if (ret_err)
			LOG_ERROR(("Failed to attach to user pipe: %s",OOBase::system_error_text(ret_err)));
		else
		{
			passed_fd.detach();

			// Tell the manager to create a new connection
			ret_err = m_pManager->new_connection(ptrSocket,uid);
		}

		int err = stream.reset();
		if (err)
			LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			size_t mark = stream.buffer()->mark_wr_ptr();

			stream.write(Omega::uint16_t(0));
			stream.write(response_id);
			stream.write(static_cast<Omega::int32_t>(ret_err));

			stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
			if (stream.last_error())
				LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				addref();

				err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
				if (err)
				{
					LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));

					release();
				}
			}
		}
	}
}

#elif defined(_WIN32)

void Registry::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
		on_message(stream);

	release();
}

void Registry::RootConnection::new_connection(OOBase::CDRStream& stream)
{
	OOBase::StackAllocator<256> allocator;

	Omega::uint16_t response_id;
	stream.read(response_id);

	OOBase::LocalString sid(allocator);
	stream.read_string(sid);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		int err = stream.reset();
		if (err)
			LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			size_t mark = stream.buffer()->mark_wr_ptr();

			stream.write(Omega::uint16_t(0));
			stream.write(response_id);

			OOBase::LocalString pipe(allocator);
			// Create named pipe with access to OOBase::Win32::SIDFromString(sid), called 'pipe'


			int ret_err = m_pManager->new_connection(sid,pipe);
			stream.write(static_cast<Omega::int32_t>(ret_err));
			if (!ret_err)
				stream.write_string(pipe);

			stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
			if (stream.last_error())
				LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				addref();

				err = m_socket->send(this,&RootConnection::on_sent,stream.buffer());
				if (err)
				{
					LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));

					release();
				}
			}
		}
	}
}

#endif

void Registry::RootConnection::on_sent(OOBase::Buffer* buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to send data to root: %s",OOBase::system_error_text(err)));

	release();
}

#if defined(HAVE_UNISTD_H)
void Registry::RootConnection::on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
#elif defined(_WIN32)
void Registry::RootConnection::on_message(OOBase::CDRStream& stream)
#endif
{
	OOServer::Root2Reg_OpCode_t op_code;
	if (!stream.read(op_code))
		LOG_ERROR(("Failed to read request opcode from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		switch (static_cast<OOServer::Root2Reg_OpCode>(op_code))
		{
		case OOServer::Root2Reg_NewConnection:
			if (recv_next())
			{
#if defined(HAVE_UNISTD_H)
				new_connection(stream,passed_fd);
#else
				new_connection(stream);
#endif
			}
			break;

		default:
			break;
		}
	}
}
