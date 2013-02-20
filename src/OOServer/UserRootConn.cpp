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

#include "OOServer_User.h"
#include "UserRootConn.h"
#include "UserManager.h"

User::RootConnection::RootConnection(Manager* pManager, OOBase::RefPtr<OOBase::AsyncSocket>& sock) :
		m_pManager(pManager),
		m_socket(sock)
{
}

User::RootConnection::~RootConnection()
{
	// Tell the manager that we have closed
	m_pManager->quit();
}

bool User::RootConnection::start()
{
#if defined(HAVE_UNISTD_H)
	OOBase::RefPtr<OOBase::Buffer> ctl_buffer = OOBase::Buffer::create(2*CMSG_SPACE(sizeof(int)),sizeof(size_t));
	if (!ctl_buffer)
		LOG_ERROR_RETURN(("Failed to allocate buffer: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

	addref();

	int err = OOBase::CDRIO::recv_msg_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_start_posix,ctl_buffer);
#elif defined(_WIN32)

	addref();

	int err = OOBase::CDRIO::recv_with_header_sync<Omega::uint16_t>(128,m_socket,this,&RootConnection::on_start_win32);
#endif
	if (err)
	{
		release();
		LOG_ERROR_RETURN(("Failed to receive from root: %s",OOBase::system_error_text(err)),false);
	}

	return true;
}

bool User::RootConnection::recv_next()
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

void User::RootConnection::on_start_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
		LOG_ERROR(("Failed to recv from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		OOBase::POSIX::SmartFD passed_fds[2];

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
						passed_fds[0] = fds[0];
						fd_start = 1;
					}
				}

				if (fd_count > 1)
				{
					err = OOBase::POSIX::set_close_on_exec(fds[1],true);
					if (!err)
					{
						passed_fds[1] = fds[1];
						fd_start = 2;
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

		if (!passed_fds[0].is_valid())
		{
			err = EINVAL;
			LOG_ERROR(("Root pipe control data invalid handle"));
		}

		if (!err)
		{
			// Read and cache any root parameters
			pid_t p1;
			stream.read(p1);

			if (stream.last_error())
				LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				err = stream.reset();
				if (err)
					LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
					size_t mark = stream.buffer()->mark_wr_ptr();

					stream.write(Omega::uint16_t(0));
					stream.write(p1);

					// Attach to the pipe
					int ret_err = 0;
					OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->attach(passed_fds[0],ret_err);
					if (ret_err)
						LOG_ERROR(("Failed to attach to registry pipe: %s",OOBase::system_error_text(ret_err)));
					else
					{
						passed_fds[0].detach();

						LOG_DEBUG(("Started!"));

						// Tell the manager to create a new connection
						ret_err = m_pManager->connect_registry(ptrSocket);
						if (!ret_err && passed_fds[1].is_valid())
						{
							// Tell the manager we have an upstream OM
						}
					}

					stream.write(static_cast<Omega::int32_t>(ret_err));

					stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
					if (stream.last_error())
						LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
					else
					{
						err = m_socket->send(this,NULL,stream.buffer());
						if (err)
							LOG_ERROR(("Failed to write response to root: %s",OOBase::system_error_text(stream.last_error())));
						else
							recv_next();
					}
				}
			}
		}
	}

	release();
}

void User::RootConnection::on_message_posix(OOBase::CDRStream& stream, OOBase::Buffer* ctl_buffer, int err)
{
	if (err)
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

void User::RootConnection::new_connection(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
{
	// Read and cache any root parameters
	pid_t pid;
	stream.read(pid);

	if (stream.last_error())
		LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		void* TODO; // Omega_ConnectChannel(passed_fd,pid) ?!?!

		passed_fd.detach();
	}
}

#elif defined(_WIN32)

void User::RootConnection::on_start_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
	{
		// Read and cache any root parameters
		DWORD p1;
		stream.read(p1);

		OOBase::StackAllocator<256> allocator;
		OOBase::LocalString strPipe(allocator);
		stream.read_string(strPipe);

		if (stream.last_error())
			LOG_ERROR(("Failed to read request from root: %s",OOBase::system_error_text(stream.last_error())));
		else
		{
			err = stream.reset();
			if (err)
				LOG_ERROR(("Failed to reset stream: %s",OOBase::system_error_text(stream.last_error())));
			else
			{
				size_t mark = stream.buffer()->mark_wr_ptr();

				stream.write(Omega::uint16_t(0));

				int ret_err = 0;

				OOBase::Timeout timeout(20,0);
				OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_pManager->m_proactor->connect(strPipe.c_str(),ret_err,timeout);
				if (ret_err != 0)
					LOG_ERROR(("Failed to connect to registry pipe: %s",OOBase::system_error_text(ret_err)));
				else
					ret_err = m_pManager->connect_registry(ptrSocket);

				stream.write(static_cast<Omega::int32_t>(ret_err));

				stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
				if (stream.last_error())
					LOG_ERROR(("Failed to write response for root: %s",OOBase::system_error_text(stream.last_error())));
				else
				{
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

void User::RootConnection::on_message_win32(OOBase::CDRStream& stream, int err)
{
	if (err)
		LOG_ERROR(("Failed to receive from root pipe: %s",OOBase::system_error_text(err)));
	else
		on_message(stream);

	release();
}

void User::RootConnection::new_connection(OOBase::CDRStream& stream)
{
	// Read and cache any root parameters
	DWORD pid;
	stream.read(pid);

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

			OOBase::StackAllocator<256> allocator;
			OOBase::LocalString pipe(allocator);

			int ret_err = connect_user(pipe);
			stream.write(static_cast<Omega::int32_t>(ret_err));
			if (!ret_err)
				stream.write_string(pipe);

			stream.replace(static_cast<Omega::uint16_t>(stream.buffer()->length()),mark);
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

#endif

#if defined(HAVE_UNISTD_H)
void User::RootConnection::on_message(OOBase::CDRStream& stream, OOBase::POSIX::SmartFD& passed_fd)
#elif defined(_WIN32)
void User::RootConnection::on_message(OOBase::CDRStream& stream)
#endif
{
	OOServer::Root2User_OpCode_t op_code;
	if (!stream.read(op_code))
		LOG_ERROR(("Failed to read request opcode from root: %s",OOBase::system_error_text(stream.last_error())));
	else
	{
		switch (static_cast<OOServer::Root2User_OpCode>(op_code))
		{
		case OOServer::Root2User_NewConnection:
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
