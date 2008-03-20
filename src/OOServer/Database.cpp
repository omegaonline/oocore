///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

#include "./OOServer_Root.h"
#include "./Database.h"

Db::Statement::~Statement()
{
	if (m_pStmt)
		sqlite3_finalize(m_pStmt);
}

int Db::Statement::step()
{
	int err = sqlite3_step(m_pStmt);
	if (err != SQLITE_ROW && err != SQLITE_DONE)
		ACE_ERROR((LM_ERROR,"%N:%l: sqlite3_step failed: %s\n",sqlite3_errmsg(sqlite3_db_handle(m_pStmt))));
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
		ACE_ERROR((LM_ERROR,"%N:%l: database open() failed: %s\n",sqlite3_errmsg(m_db)));
}

int Db::Database::open(const ACE_CString& strDb)
{
	if (!sqlite3_threadsafe())
		ACE_ERROR_RETURN((LM_ERROR,"Sqlite is not built threadsafe!\n"),-1);

    if (m_db)
		ACE_ERROR_RETURN((LM_ERROR,"Database already open!\n"),-1);

	int err = sqlite3_open(strDb.c_str(),&m_db);
	if (err != SQLITE_OK)
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: database open() failed: %s\n",sqlite3_errmsg(m_db)),-1);

	return 0;
}

int Db::Database::exec(const char* szSQL)
{
	int err = sqlite3_exec(m_db,szSQL,NULL,0,NULL);
	if (err != SQLITE_OK)
		ACE_ERROR((LM_ERROR,"%N:%l: sqlite3_exec failed: %s\n",sqlite3_errmsg(m_db)));
	return err;
}

sqlite3* Db::Database::database()
{
	return m_db;
}

ACE_Refcounted_Auto_Ptr<Db::Transaction,ACE_Null_Mutex> Db::Database::begin_transaction(const char* pszType)
{
	int err = 0;
	if (pszType)
		err = sqlite3_exec(m_db,pszType,NULL,0,NULL);
	else
		err = sqlite3_exec(m_db,"BEGIN TRANSACTION;",NULL,0,NULL);
	if (err != SQLITE_OK)
		return 0;

	ACE_Refcounted_Auto_Ptr<Transaction,ACE_Null_Mutex> ptrTrans;
	ACE_NEW_NORETURN(ptrTrans,Transaction(m_db));
	if (ptrTrans.null())
	{
		sqlite3_exec(m_db,"ROLLBACK;",NULL,0,NULL);
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %m\n")));
	}

	return ptrTrans;
}

ACE_Refcounted_Auto_Ptr<Db::Statement,ACE_Null_Mutex> Db::Database::prepare_statement(const char* pszStatement, ...)
{
	va_list ap;
	va_start(ap,pszStatement);
	char* pszBuf = sqlite3_vmprintf(pszStatement,ap);
	va_end(ap);

	if (!pszBuf)
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: sqlite3_vmprintf failed: %s\n",sqlite3_errmsg(m_db)),0);

	sqlite3_stmt* pStmt = 0;
	int err = sqlite3_prepare_v2(m_db,pszBuf,-1,&pStmt,NULL);
	sqlite3_free(pszBuf);

	if (err != SQLITE_OK)
		ACE_ERROR_RETURN((LM_ERROR,"%N:%l: sqlite3_prepare_v2 failed: %s\n",sqlite3_errmsg(m_db)),0);

	ACE_Refcounted_Auto_Ptr<Statement,ACE_Null_Mutex> ptrStmt;
	ACE_NEW_NORETURN(ptrStmt,Statement(pStmt));
	if (ptrStmt.null())
	{
		sqlite3_finalize(pStmt);
		ACE_ERROR((LM_ERROR,ACE_TEXT("%N:%l: %m\n")));
	}

	return ptrStmt;
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
