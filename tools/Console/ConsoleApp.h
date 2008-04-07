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
#ifndef CONSOLE_APP_H
#define CONSOLE_APP_H
#include "ConsoleTools.h"
template <class T>
class ConsoleApp
{
public:
	ConsoleApp() {}
	int run(int argc,char **argv)
	{
		// handle app args
		if(options(argc,argv))
			usage();
		
		// run the state machine
		else if(!m_ct.run())
			return EXIT_SUCCESS;
		
		// all done time to go
	 	return EXIT_FAILURE; 
	}
private:
	T m_ct;

protected:
	void license(void)
	{
	 	TMP_PRINTF("%s\n","License can be found at http://www.gnu.org/licenses/");
	}
	
	void usage(void)
	{
	static const char *msg = "\t --help		shows these options\n"
				 "\t --license		shows gpl license\n"
				 "\t --source filename	loads state table from filename\n";
	
		TMP_PRINTF("\n%s",msg);
	}

	int options(int argc,char **argv)
	{
	int retval=1;
		// handle command line arguments
		for(int i = -retval; ++i < argc;)
		{
			// print usage instructions
			if(!strcmp("--help",argv[i]))
			{
				usage();
				exit(EXIT_SUCCESS);
			}
			
			// print gpl license	
			else if(!strcmp("--license",argv[i]))
			{
				license();
				exit(EXIT_SUCCESS);
			}
			
			// load a file of state transitions to drive the application
			else if(!strcmp("--source",argv[i]))
			{
				// bail if missing a filename
				if(i+1 >= argc)
				{
					TMP_PRINTF(TMP_ERRMSG,"missing filename");
					exit(EXIT_FAILURE);
				}
			
				// load the FSM tables from source file
				retval = m_ct.load_state_table(argv[i+1]);
			}
			
		}
		return retval;	
	}
	
};
#endif /*ndef CONSOLE_APP_H */
