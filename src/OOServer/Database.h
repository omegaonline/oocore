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

#ifndef OOSERVER_DATABASE_H_INCLUDED_
#define OOSERVER_DATABASE_H_INCLUDED_

#include "./OOServer_Root.h"

#include <sqlite3.h>

namespace Db
{
	class Statement
	{
		friend class Database;
	public:
		~Statement();

		int step();
		int column_int(int iCol);
		const char* column_text(int iCol);
		sqlite3_int64 column_int64(int iCol);
		const void* column_blob(int iCol);
		int column_bytes(int iCol);

		sqlite3_stmt* statement();

	private:
		Statement(sqlite3_stmt* pStmt);
		Statement(const Statement&) {};
		Statement& operator = (const Statement&) { return *this; };

		sqlite3_stmt* m_pStmt;
	};

	class Transaction
	{
		friend class Database;

	public:
		~Transaction();

		int commit();
		int rollback();

	private:
		Transaction(sqlite3* db);
		Transaction(const Transaction&) {};
		Transaction& operator = (const Transaction&) { return *this; };

		sqlite3* m_db;
	};

	class Database
	{
	public:
		Database();
		~Database();

		int open(const ACE_CString& strDb);
		int exec(const char* szSQL);

		sqlite3* database();

		ACE_Refcounted_Auto_Ptr<Transaction,ACE_Thread_Mutex> begin_transaction(const char* pszType = 0);
		ACE_Refcounted_Auto_Ptr<Statement,ACE_Thread_Mutex> prepare_statement(const char* pszStatement, ...);

	private:
		sqlite3* m_db;
	};
}

#endif // OOSERVER_DATABASE_H_INCLUDED_
