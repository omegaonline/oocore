///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
//
// This file is part of OOCore/libdb, the Omega Online Core db library.
//
// OOCore/libdb is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore/libdb is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore/libdb.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <OOBase/GlobalNew.h>
#include <OOBase/Logger.h>

#include "Database.h"

#if defined(HAVE_SQLITE3_H) || defined(HAVE_SQLITE3_AMALGAMATION)

Db::Statement::Statement(sqlite3_stmt* pStmt) :
		m_pStmt(pStmt)
{ }

Db::Statement::~Statement()
{
	if (m_pStmt)
		sqlite3_finalize(m_pStmt);
}

int Db::Statement::prepare(Database& db, const char* pszStatement, ...)
{
	va_list ap;
	va_start(ap,pszStatement);
	char* pszBuf = sqlite3_vmprintf(pszStatement,ap);
	va_end(ap);

	if (!pszBuf)
		LOG_ERROR_RETURN(("sqlite3_vmprintf failed: %s",sqlite3_errmsg(db.m_db)),sqlite3_errcode(db.m_db));

	if (m_pStmt)
	{
		sqlite3_finalize(m_pStmt);
		m_pStmt = NULL;
	}
	
	int err = sqlite3_prepare_v2(db.m_db,pszBuf,-1,&m_pStmt,NULL);
	sqlite3_free(pszBuf);

	if (err != SQLITE_OK)
		LOG_ERROR_RETURN(("sqlite3_prepare_v2 failed: %s",sqlite3_errmsg(db.m_db)),err);

	return err;
}

int Db::Statement::step()
{
	int err = sqlite3_step(m_pStmt);
	if (err != SQLITE_ROW && err != SQLITE_DONE && err != SQLITE_READONLY)
		LOG_ERROR(("sqlite3_step failed: %s",sqlite3_errmsg(sqlite3_db_handle(m_pStmt))));
	return err;
}

int Db::Statement::reset(bool log)
{
	int err = sqlite3_reset(m_pStmt);
	if (err != SQLITE_OK && log)
		LOG_ERROR(("sqlite3_reset failed: %s",sqlite3_errmsg(sqlite3_db_handle(m_pStmt))));
	
	return err;
}

int Db::Statement::column_int(int iCol)
{
	return sqlite3_column_int(m_pStmt,iCol);
}

const char* Db::Statement::column_text(int iCol)
{
	return reinterpret_cast<const char*>(sqlite3_column_text(m_pStmt,iCol));
}

sqlite3_int64 Db::Statement::column_int64(int iCol)
{
	return sqlite3_column_int64(m_pStmt,iCol);
}

const void* Db::Statement::column_blob(int iCol)
{
	return sqlite3_column_blob(m_pStmt,iCol);
}

int Db::Statement::column_bytes(int iCol)
{
	return sqlite3_column_bytes(m_pStmt,iCol);
}

int Db::Statement::bind_int64(int index, const sqlite3_int64& val)
{
	return sqlite3_bind_int64(m_pStmt,index,val);
}

int Db::Statement::bind_string(int index, const char* val, size_t len)
{
	return sqlite3_bind_text(m_pStmt,index,val,static_cast<int>(len),NULL);
}

Db::Database::Database() :
		m_db(NULL)
{
	assert(sqlite3_threadsafe());
}

Db::Database::~Database()
{
	if (m_db)
	{
		// Close all prepared statements...
		sqlite3_stmt* pStmt;
		while ((pStmt = sqlite3_next_stmt(m_db,NULL)) != NULL)
			sqlite3_finalize(pStmt);

		// Now close the db
		if (sqlite3_close(m_db) != SQLITE_OK)
			LOG_ERROR(("sqlite3_close failed: %s",sqlite3_errmsg(m_db)));
	}
}

bool Db::Database::open(const char* pszDb, int flags)
{
	assert(!m_db);

	int err = sqlite3_open_v2(pszDb,&m_db,SQLITE_OPEN_FULLMUTEX | flags,NULL);
	if (err != SQLITE_OK)
	{
		if (!m_db)
			LOG_ERROR_RETURN(("sqlite3_open failed: Out of memory"),false);
		else
		{
			LOG_ERROR(("sqlite3_open(%s) failed: %s",pszDb,sqlite3_errmsg(m_db)));
			sqlite3_close(m_db);
			m_db = NULL;
			return false;
		}
	}

	sqlite3_busy_timeout(m_db,500);

	return true;
}

int Db::Database::exec(const char* szSQL)
{
	int err = sqlite3_exec(m_db,szSQL,NULL,NULL,NULL);
	if (err != SQLITE_OK && err != SQLITE_READONLY)
		LOG_ERROR(("sqlite3_exec failed: %s",sqlite3_errmsg(m_db)));
	return err;
}

sqlite3_int64 Db::Database::last_insert_rowid()
{
	return sqlite3_last_insert_rowid(m_db);
}

Db::Transaction::Transaction(Database& db) :
		m_db(db.m_db)
{ }

Db::Transaction::~Transaction()
{
	if (m_db)
		rollback();
}

int Db::Transaction::begin(const char* pszType)
{
	int err = 0;
	if (pszType)
		err = sqlite3_exec(m_db,pszType,NULL,NULL,NULL);
	else
		err = sqlite3_exec(m_db,"BEGIN TRANSACTION;",NULL,NULL,NULL);

	if (err != SQLITE_OK)
		LOG_ERROR_RETURN(("sqlite3_exec(%s) failed: %s",pszType ? pszType : "BEGIN TRANSACTION",sqlite3_errmsg(m_db)),err);
	
	return 0;
}

int Db::Transaction::commit()
{
	int err = sqlite3_exec(m_db,"COMMIT;",NULL,NULL,NULL);
	if (err == SQLITE_OK)
		m_db = NULL;
	else
		LOG_ERROR(("sqlite3_exec(COMMIT) failed: %s",sqlite3_errmsg(m_db)));
		
	return err;
}

int Db::Transaction::rollback()
{
	int err = sqlite3_exec(m_db,"ROLLBACK;",NULL,NULL,NULL);
	if (err == SQLITE_OK)
		m_db = NULL;
	else
		LOG_ERROR(("sqlite3_exec(ROLLBACK) failed: %s",sqlite3_errmsg(m_db)));
		
	return err;
}

#endif // defined(HAVE_SQLITE3_H) || defined(HAVE_SQLITE3_AMALGAMATION)
