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

#include "PosixSocket.h"

#if (!defined(_WIN32) && defined(HAVE_SYS_SOCKET_H)) || defined(DOXYGEN)

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#if defined(HAVE_SYS_FCNTL_H)
#include <sys/fcntl.h>
#endif /* HAVE_SYS_FCNTL_H */

#if defined(HAVE_SYS_UN_H)
#include <sys/un.h>
#endif /* HAVE_SYS_UN_H */

int OOBase::POSIX::fcntl_addfd(int fd, int flags)
{
	int oldflags = fcntl(fd,F_GETFD);
	if (oldflags == -1 ||
			fcntl(fd,F_SETFD,oldflags | flags) == -1)
	{
		return errno;
	}

	return 0;
}

int OOBase::POSIX::fcntl_addfl(int fd, int flags)
{
	int oldflags = fcntl(fd,F_GETFL);
	if (oldflags == -1 ||
			fcntl(fd,F_SETFL,oldflags | flags) == -1)
	{
		return errno;
	}

	return 0;
}

OOBase::LocalSocket::uid_t OOBase::POSIX::LocalSocket::get_uid()
{
#if defined(HAVE_GETPEEREID)
	/* OpenBSD style:  */
	uid_t uid;
	gid_t gid;
	if (getpeereid(m_fd, &uid, &gid) != 0)
	{
		/* We didn't get a valid credentials struct. */
		OOBase_CallCriticalFailure(errno);
		return -1;
	}
	return uid;

#elif defined(HAVE_SO_PEERCRED)
	/* Linux style: use getsockopt(SO_PEERCRED) */
	ucred peercred;
	socklen_t so_len = sizeof(peercred);

	if (getsockopt(m_fd, SOL_SOCKET, SO_PEERCRED, &peercred, &so_len) != 0 || so_len != sizeof(peercred))
	{
		/* We didn't get a valid credentials struct. */
		OOBase_CallCriticalFailure(errno);
		return -1;
	}
	return peercred.uid;

#elif defined(HAVE_GETPEERUCRED)
	/* Solaris > 10 */
	ucred_t* ucred = NULL; /* must be initialized to NULL */
	if (getpeerucred(m_fd, &ucred) != 0)
	{
		OOBase_CallCriticalFailure(errno);
		return -1;
	}

	uid_t uid;
	if ((uid = ucred_geteuid(ucred)) == -1)
	{
		int err = errno;
		ucred_free(ucred);
		OOBase_CallCriticalFailure(err);
		return -1;
	}
	return uid;

#elif (defined(HAVE_STRUCT_CMSGCRED) || defined(HAVE_STRUCT_FCRED) || defined(HAVE_STRUCT_SOCKCRED)) && defined(HAVE_LOCAL_CREDS)

	/*
	* Receive credentials on next message receipt, BSD/OS,
	* NetBSD. We need to set this before the client sends the
	* next packet.
	*/
	int on = 1;
	if (setsockopt(m_fd, 0, LOCAL_CREDS, &on, sizeof(on)) != 0)
	{
		OOBase_CallCriticalFailure(errno);
		return -1;
	}

	/* Credentials structure */
#if defined(HAVE_STRUCT_CMSGCRED)
	typedef cmsgcred Cred;
	#define cruid cmcred_uid
#elif defined(HAVE_STRUCT_FCRED)
	typedef fcred Cred;
	#define cruid fc_uid
#elif defined(HAVE_STRUCT_SOCKCRED)
	typedef sockcred Cred;
	#define cruid sc_uid
#endif
	/* Compute size without padding */
	char cmsgmem[ALIGN(sizeof(struct cmsghdr)) + ALIGN(sizeof(Cred))];   /* for NetBSD */

	/* Point to start of first structure */
	struct cmsghdr* cmsg = (struct cmsghdr*)cmsgmem;
	struct iovec iov;

	msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = (char *) cmsg;
	msg.msg_controllen = sizeof(cmsgmem);
	memset(cmsg, 0, sizeof(cmsgmem));

	/*
	 * The one character which is received here is not meaningful; its
	 * purposes is only to make sure that recvmsg() blocks long enough for the
	 * other side to send its credentials.
	 */
	char buf;
	iov.iov_base = &buf;
	iov.iov_len = 1;

	if (recvmsg(m_fd, &msg, 0) < 0 || cmsg->cmsg_len < sizeof(cmsgmem) || cmsg->cmsg_type != SCM_CREDS)
	{
		OOBase_CallCriticalFailure(errno);
		return -1;
	}

	Cred* cred = (Cred*)CMSG_DATA(cmsg);
	return cred->cruid;
#else
	// We can't handle this situation
	#error Fix me!
	return -1;
#endif
}

OOBase::LocalSocket* OOBase::LocalSocket::connect_local(const std::string& path, int* perr, const timeval_t* wait)
{
	int fd;
	if ((fd = socket(AF_UNIX,SOCK_STREAM,0)) == -1)
	{
		*perr = errno;
		return 0;
	}

#if 0
	// Stop SIG_PIPE
	int on = 1;
	if (setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE &on, sizeof(on)) != 0)
	{
		*perr = errno;
		::close(fd);
		return 0;
	}
#endif

	// Add FD_CLOEXEC
	*perr = POSIX::fcntl_addfd(fd,FD_CLOEXEC);
	if (*perr != 0)
	{
		::close(fd);
		return 0;
	}

	sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	memset(addr.sun_path,0,sizeof(addr.sun_path));
	path.copy(addr.sun_path,sizeof(addr.sun_path)-1);

	if (connect(fd,(sockaddr*)(&addr),sizeof(addr)) != 0)
	{
		*perr = errno;
		::close(fd);
		return 0;
	}

	OOBase::POSIX::LocalSocket* pSocket = 0;
	OOBASE_NEW(pSocket,OOBase::POSIX::LocalSocket(fd,path));
	if (!pSocket)
	{
		*perr = ENOMEM;
		::close(fd);
		return 0;
	}

	return pSocket;
}

OOBase::POSIX::SocketImpl::SocketImpl(int fd) :
		m_fd(fd)
{
}

OOBase::POSIX::SocketImpl::~SocketImpl()
{
	close();
}

int OOBase::POSIX::SocketImpl::send(const void* buf, size_t len, const OOBase::timeval_t* /*timeout*/)
{
	ssize_t sent = ::send(m_fd,buf,len,0);
	if (sent == -1)
		return errno;
	else
		return 0;
}

size_t OOBase::POSIX::SocketImpl::recv(void* buf, size_t len, int* perr, const OOBase::timeval_t* /*timeout*/)
{
	ssize_t read = ::recv(m_fd,buf,len,0);
	if (read != -1)
		return static_cast<size_t>(read);

	*perr = errno;
	return 0;
}

void OOBase::POSIX::SocketImpl::close()
{
	::close(m_fd);
	m_fd = -1;
}

#endif // !defined(_WIN32) && defined(HAVE_SYS_SOCKET_H)
