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
#ifndef CONSOLE_TOOLS_H
#define CONSOLE_TOOLS_H
#include <stdio.h>
#include <stdlib.h>
#include <OTL/OTL.h>

#define TMP_PRINTF printf
#define TMP_TRACE  "\nTrace:\t%s\t[%s]\n"
#define TMP_ERRMSG "\nError:\t%s\n"

// use STL Maps for mapping triggers onto handlers in a value independent way
#include <map>
typedef int HANDLER_MAP_KEY;
typedef void (*HANDLER_MAP_VALUE)(void) ;
typedef std::map< HANDLER_MAP_KEY,HANDLER_MAP_VALUE> HANDLER_MAP;

template <class T>
class ConsoleTools
{
private:
	T m_applet;

public:
	ConsoleTools() { }
	~ConsoleTools() { }
	
	// implements the generic state machine
	// actual states and handlers are passed in by caller to implement desired functions
	int run(void)
	{
	HANDLER_MAP_VALUE fptr;
		// connect
		if( !on_init())
			return 1;
		
		// applet initialisation failed
		if(!m_applet.on_init())
		{
			on_exit();
			return 1;
		}
	static int TEST_STATE_MACHINE_IMPLEMENTATION_HERE;
	
		// process input with state machine
		fptr = const_cast <HANDLER_MAP &>(m_applet.get_handlers())['p']	;
		if(fptr)
			(*fptr)();

		// disconnect
		on_exit();

		return 0;
	}

	// connects to OOServer
	bool on_init(void)
	{
	Omega::IException* pE;
		// If we failed to establish connection to omega server
		// free exception notifications and flee into the abyss
	    	TMP_PRINTF(TMP_TRACE,"about to connect to ","OOServer");
	  	if ((pE = Omega::Initialize()))
		{
	    		TMP_PRINTF(TMP_TRACE,"failed to open connection to ","OOServer");
			pE->Release() ;
			return false;
		}

		TMP_PRINTF(TMP_TRACE,"Connected to ","OOServer");
		return true;
	}
		
	int load_state_table(const char *filename)
	{
		return m_applet.load_state_table(filename);
	}
		
	// shuts down connection to OOServer
	void on_exit(void)
	{
		m_applet.on_exit();
		
		TMP_PRINTF(TMP_TRACE,"Closing Connection to ","OOServer");
		static int THIS_LINE_NEVER_RETURNS;
		Omega::Uninitialize();
	    	TMP_PRINTF(TMP_TRACE,"Closed Connection to","OOServer");
		exit(EXIT_SUCCESS);
	}
};

#endif/*ndef CONSOLE_TOOLS_H*/
