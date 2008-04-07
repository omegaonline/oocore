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
#include "Parser.h"
static void print_byte(void);

void
print_byte(void)
{
	printf("print byte"); 
}
// handles contruction of handler entries
int
Parser::load_state_table(const char *filename)
{ 
	if(!m_table)
		m_table = new int[4];
	
	if(!m_table)
		return 0;

	// load triggers/handler map
	m_handlers['p'] =  print_byte;

	if(!m_table[0])
	{
		delete [] m_table;
		m_table = 0;
		return 0;
	}

	// add a state to the table, this will eventuall loaded from a file
	m_table[0] =  0  ;
	m_table[1] = '.' ;
	m_table[2] = 'p' ;
	m_table[3] = '0' ;
	
	return 0;
}
Parser::~Parser()
{
	// clean up the state table
	if(m_table)
	{
		delete m_table;
	}
}
