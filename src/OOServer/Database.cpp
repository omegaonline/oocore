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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "OOServer_Root.h"
#include "Database.h"

Db::Statement::~Statement()
{
	if (m_pStmt)
		sqlite3_finalize(m_pStmt);
}

int Db::Statement::step()
{
	int err = sqlite3_step(m_pStmt);
	if (err != SQLITE_ROW && err != SQLITE_DONE)
		LOG_ERROR(("sqlite3_step failed: %s",sqlite3_errmsg(sqlite3_db_handle(m_pStmt))));
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

sqlite3_stmt* Db::Statement::statement()
{
	return m_pStmt;
}

Db::Statement::Statement(sqlite3_stmt* pStmt) :
   m_pStmt(pStmt)
{ }


Db::Transaction::Transaction(sqlite3* db) :
   m_db(db)
{
}

Db::Database::Database() :
	m_db(0)
{
}

Db::Database::~Database()
{
	if (sqlite3_close(m_db) != SQLITE_OK)
		LOG_ERROR(("sqlite3_close failed: %s",sqlite3_errmsg(m_db)));
}

bool Db::Database::open(const char* pszDb)
{
	if (!sqlite3_threadsafe())
		LOG_ERROR_RETURN(("SQLite is not built threadsafe"),false);

    assert(!m_db);
		
	int err = sqlite3_open(pszDb,&m_db);
	if (err != SQLITE_OK)
		LOG_ERROR_RETURN(("sqlite3_open(%s) failed: %s",pszDb,sqlite3_errmsg(m_db)),false);

	return true;
}

int Db::Database::exec(const char* szSQL)
{
	int err = sqlite3_exec(m_db,szSQL,NULL,0,NULL);
	if (err != SQLITE_OK)
		LOG_ERROR(("sqlite3_exec failed: %s",sqlite3_errmsg(m_db)));
	return err;
}

sqlite3* Db::Database::database()
{
	return m_db;
}

OOBase::SmartPtr<Db::Transaction> Db::Database::begin_transaction(const char* pszType)
{
	int err = 0;
	if (pszType)
		err = sqlite3_exec(m_db,pszType,NULL,0,NULL);
	else
		err = sqlite3_exec(m_db,"BEGIN TRANSACTION;",NULL,0,NULL);
	if (err != SQLITE_OK)
		return OOBase::SmartPtr<Db::Transaction>(0);

	Transaction* pTrans = 0;
	OOBASE_NEW(pTrans,Transaction(m_db));
	if (!pTrans)
	{
		sqlite3_exec(m_db,"ROLLBACK;",NULL,0,NULL);
		LOG_ERROR(("Out of memory"));
	}

	return OOBase::SmartPtr<Transaction>(pTrans);
}

OOBase::SmartPtr<Db::Statement> Db::Database::prepare_statement(const char* pszStatement, ...)
{
	va_list ap;
	va_start(ap,pszStatement);
	char* pszBuf = sqlite3_vmprintf(pszStatement,ap);
	va_end(ap);

	if (!pszBuf)
	{
		LOG_ERROR(("sqlite3_vmprintf failed: %s",sqlite3_errmsg(m_db)));
		return OOBase::SmartPtr<Db::Statement>();
	}

	sqlite3_stmt* pStmt = 0;
	int err = sqlite3_prepare_v2(m_db,pszBuf,-1,&pStmt,NULL);
	sqlite3_free(pszBuf);

	if (err != SQLITE_OK)
	{
		LOG_ERROR(("sqlite3_prepare_v2 failed: %s",sqlite3_errmsg(m_db)));
		return OOBase::SmartPtr<Db::Statement>();
	}

	Statement* pSt = 0;
	OOBASE_NEW(pSt,Statement(pStmt));
	if (!pSt)
	{
		sqlite3_finalize(pStmt);
		LOG_ERROR(("Out of memory"));
	}

	return OOBase::SmartPtr<Statement>(pSt);
}

Db::Transaction::~Transaction()
{
	if (m_db)
		sqlite3_exec(m_db,"ROLLBACK;",NULL,0,NULL);
}

int Db::Transaction::commit()
{
	int err = sqlite3_exec(m_db,"COMMIT;",NULL,0,NULL);
	if (err == SQLITE_OK)
		m_db = 0;

	return err;
}

int Db::Transaction::rollback()
{
	int err = sqlite3_exec(m_db,"ROLLBACK;",NULL,0,NULL);
	if (err == SQLITE_OK)
		m_db = 0;

	return err;
}
