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

#ifndef OOCORE_LIBDB_DATABASE_H_INCLUDED_
#define OOCORE_LIBDB_DATABASE_H_INCLUDED_

#include "Common.h"

#if !defined(HAVE_SQLITE3_H) && !defined(HAVE_SQLITE3_AMALGAMATION)
#error SQLite3 required!
#else

#include <sqlite3.h>

namespace Db
{
	class Database
	{
		friend class Statement;
		friend class Transaction;

	public:
		Database();
		~Database();

		bool open(const char* pszDb, int flags);
		int exec(const char* pszSQL);

		sqlite3_int64 last_insert_rowid();
		
	private:
		sqlite3* m_db;

		Database(const Database&);
		Database& operator = (const Database&);
	};

	class Statement
	{
	public:
		Statement(sqlite3_stmt* pStmt = NULL);
		~Statement();

		int prepare(Database& db, const char* pszStatement, ...);

		int step();
		int reset(bool log = true);

		int column_int(int iCol);
		const char* column_text(int iCol);
		sqlite3_int64 column_int64(int iCol);
		const void* column_blob(int iCol);
		int column_bytes(int iCol);

		int bind_int64(int index, const sqlite3_int64& val);
		int bind_string(int index, const char* val, size_t len);

	private:
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
			m_stmt.reset(false);
		}

	private:
		Resetter(const Resetter&);
		Resetter& operator = (const Resetter&);

		Statement& m_stmt;
	};

	class Transaction
	{
	public:
		Transaction(Database& db);
		~Transaction();

		int begin(const char* pszType = NULL);
		int commit();
		int rollback();

	private:
		Transaction(const Transaction&);
		Transaction& operator = (const Transaction&);

		sqlite3* m_db;
	};
}

#endif // #if defined(HAVE_SQLITE3_H) || defined(HAVE_SQLITE3_AMALGAMATION)

#endif // OOCORE_LIBDB_DATABASE_H_INCLUDED_
