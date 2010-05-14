///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOSVRBASE_DATABASE_H_INCLUDED_
#define OOSVRBASE_DATABASE_H_INCLUDED_

#include "SmartPtr.h"

#if !defined(HAVE_SQLITE3) && !defined(HAVE_SQLITE3_AMALGAMATION)
#error SQLite3 required!
#else

#include <sqlite3.h>

namespace OOSvrBase
{
	namespace Db
	{
		class Statement
		{
			friend class Database;

		public:
			~Statement();

			int step();
			int reset();

			int column_int(int iCol);
			const char* column_text(int iCol);
			sqlite3_int64 column_int64(int iCol);
			const void* column_blob(int iCol);
			int column_bytes(int iCol);

			int bind_int64(int index, const sqlite3_int64& val);
			int bind_string(int index, const std::string& val);

			sqlite3_stmt* statement();

		private:
			Statement(sqlite3_stmt* pStmt);
			Statement(const Statement&);
			Statement& operator = (const Statement&);

			sqlite3_stmt* m_pStmt;
		};

		class Resetter
		{
		public:
			Resetter(Statement& stmt) : m_stmt(stmt)
			{}

			~Resetter()
			{
				m_stmt.reset();
			}

		private:
			Resetter(const Resetter&);
			Resetter& operator = (const Resetter&);

			Statement& m_stmt;
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
			Transaction(const Transaction&);
			Transaction& operator = (const Transaction&);

			sqlite3* m_db;
		};

		class Database
		{
		public:
			Database();
			~Database();

			bool open(const char* pszDb, int flags);
			int exec(const char* pszSQL);

			sqlite3* database();

			int begin_transaction(OOBase::SmartPtr<Transaction>& ptrTrans, const char* pszType = 0);
			int prepare_statement(OOBase::SmartPtr<Statement>& ptrStmt, const char* pszStatement, ...);

		private:
			sqlite3* m_db;

			Database(const Database&);
			Database& operator = (const Database&);
		};
	}
}

#endif // #if defined(HAVE_SQLITE3) || defined(HAVE_SQLITE3_AMALGAMATION)

#endif // OOSVRBASE_DATABASE_H_INCLUDED_
