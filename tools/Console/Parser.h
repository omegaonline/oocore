///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Jamal M. Natour
//
// This file is part of the OmegaOnline ConsoleTools package.
//
// ConsoleTools is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// ConsoleTools is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with ConsoleTools.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////
#ifndef PARSER_H
#define PARSER_H
#include "ConsoleTools.h"

class Parser
{
private:
	HANDLER_MAP m_handlers;
	int       *m_table; 
public:
	Parser() {m_table=0;}
	~Parser();
	
	bool on_init(void) { return true;}
	void on_exit(void) {}

	int load_state_table(const char *filename);
	const HANDLER_MAP & get_handlers(void) const { return m_handlers; }
};

#endif/*ndef CONSOLE_TOOLS_H*/
