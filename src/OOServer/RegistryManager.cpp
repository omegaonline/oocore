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
#include "RegistryManager.h"
#include "RegistryRootConn.h"

#include <stdlib.h>

#if defined(_WIN32)
#include <aclapi.h>
#include <sddl.h>
#endif

Registry::Manager::Manager() :
		m_proactor(NULL)
{
}

Registry::Manager::~Manager()
{
}

int Registry::Manager::run(const OOBase::LocalString& strPipe)
{
	int ret = EXIT_FAILURE;
	int err = 0;
	m_proactor = OOBase::Proactor::create(err);
	if (err)
		LOG_ERROR(("Failed to create proactor: %s",OOBase::system_error_text(err)));
	else
	{
		// Only one thread at first...
		err = m_proactor_pool.run(&run_proactor,m_proactor,1);
		if (err != 0)
			LOG_ERROR(("Thread pool create failed: %s",OOBase::system_error_text(err)));
		else
		{
			if (connect_root(strPipe))
			{
				OOBase::Logger::log(OOBase::Logger::Information,APPNAME " started successfully");

				ret = EXIT_SUCCESS;

				// Wait for quit
				wait_for_quit();
			}

			m_proactor->stop();
			m_proactor_pool.join();
		}

		OOBase::Proactor::destroy(m_proactor);
	}

	if (Registry::is_debug() /*&& ret != EXIT_SUCCESS*/)
	{
		OOBase::Logger::log(OOBase::Logger::Debug,"Pausing to let you read the messages...");

		// Give us a chance to read the errors!
		OOBase::Thread::sleep(15000);
	}

	return ret;
}

int Registry::Manager::run_proactor(void* p)
{
	int err = 0;
	return static_cast<OOBase::Proactor*>(p)->run(err);
}

bool Registry::Manager::connect_root(const OOBase::LocalString& strPipe)
{
	int err = 0;

#if defined(_WIN32)
	// Use a named pipe
	OOBase::Timeout timeout(20,0);
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_proactor->connect(strPipe.c_str(),err,timeout);
	if (err != 0)
		LOG_ERROR_RETURN(("Failed to connect to root pipe: %s",OOBase::system_error_text(err)),false);

#else

	// Use the passed fd
	int fd = atoi(strPipe.c_str());
	OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket = m_proactor->attach(fd,err);
	if (err != 0)
	{
		OOBase::POSIX::close(fd);
		LOG_ERROR_RETURN(("Failed to attach to root pipe: %s",OOBase::system_error_text(err)),false);
	}

#endif

	OOBase::RefPtr<Registry::RootConnection> conn = new (std::nothrow) Registry::RootConnection(this,ptrSocket);
	if (!conn)
		LOG_ERROR_RETURN(("Failed to create root connection: %s",OOBase::system_error_text()),false);

	return conn->start();
}

int Registry::Manager::on_start(const OOBase::LocalString& strDb, size_t nThreads, const OOBase::Table<OOBase::LocalString,OOBase::LocalString,OOBase::AllocatorInstance>& tabSettings)
{
	int err = m_strDb.assign(strDb);
	if (err)
		LOG_ERROR_RETURN(("Failed to assign string: %s",OOBase::system_error_text(err)),err);

	LOG_DEBUG(("Open Database: %s",strDb.c_str()));

	err = open_database();
	if (!err && !tabSettings.empty())
	{
		void* TODO;

		// err = insert_system_settings(tabSettings);
	}

	// Start the extra threads
	if (!err)
		m_proactor_pool.run(&open_run_proactor,this,nThreads - 1);

	return err;
}

int Registry::Manager::open_database()
{
	// Create a new system database
		/*	m_registry = new (std::nothrow) Db::Hive(this,strDb.c_str());
			if (!m_registry)
				LOG_ERROR_RETURN(("Failed to create registry hive: %s",OOBase::system_error_text(ERROR_OUTOFMEMORY)),false);

			return m_registry->open(SQLITE_OPEN_READWRITE);*/

	return 0;
}

int Registry::Manager::open_run_proactor(void* p)
{
	Registry::Manager* pThis = static_cast<Registry::Manager*>(p);

	int err = pThis->open_database();
	if (!err)
		err = run_proactor(pThis->m_proactor);
	return err;
}

int Registry::Manager::new_connection(OOBase::RefPtr<OOBase::AsyncSocket> ptrSocket, uid_t uid)
{

	return 0;
}

/*void Registry::Manager::on_root_message(OOBase::Buffer* buffer, int err)
{
	OOBase::CDRStream stream(buffer);

	if (err)
		LOG_ERROR(("Failed to receive from root stream: %s",OOBase::system_error_text(err)));

	while (!err && buffer->length() >= sizeof(Omega::uint16_t))
	{
		// Get the buffer length before we read the message length because of alignment
		size_t blen = buffer->length();

		Omega::uint16_t len = 0;
		if (!stream.read(len))
		{
			err = stream.last_error();
			LOG_ERROR(("Failed to read from root stream: %s",OOBase::system_error_text(err)));
			break;
		}

		// Check we have a complete message
		if (blen < len)
		{
			// Make room for the rest
			err = buffer->space(len - blen);
			if (err)
				LOG_ERROR(("Failed to grow buffer: %s",OOBase::system_error_text(err)));
			break;
		}

		OOBase::CDRStream response;

		// Record where we wrote the length, then write a dummy value
		size_t mark = response.buffer()->mark_wr_ptr();
		if (!response.write(Omega::uint16_t(0)))
		{
			err = stream.last_error();
			LOG_ERROR(("Failed to write root response: %s",OOBase::system_error_text(err)));
			break;
		}

		// Create a new connection
		err = new_connection(stream,response);
		if (!err)
		{
			// Update the length in the response
			response.replace(Omega::uint16_t(response.buffer()->length()),mark);

			err = m_root_socket->send(NULL,NULL,response.buffer());
			if (err)
				LOG_ERROR(("Failed to send response: %s",OOBase::system_error_text(err)));
		}

		if (!err)
		{
			// Compact the read buffer to ensure alignment for the next read
			err = buffer->compact(OOBase::CDRStream::MaxAlignment);
			if (err)
				LOG_ERROR(("Failed to compact buffer: %s",OOBase::system_error_text(err)));
		}
	}

	if (!err)
	{
		// Perform another recv
		err = m_root_socket->recv(this,&Manager::on_root_message,buffer);
		if (err)
			LOG_ERROR(("Failed to receive from root: %s",OOBase::system_error_text(err)));
	}

	// Close the root socket if we fail
	if (err)
	{
		void* TODO; // Close all open sockets;

		m_root_socket = NULL;
	}
}
*/
