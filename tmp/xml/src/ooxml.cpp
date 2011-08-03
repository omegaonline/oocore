///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOXML, the Omega Online XML library.
//
// OOXML is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOXML is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOXML.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <stdio.h>

#include "Tokenizer.h"

static FILE* f = NULL;

int main( int argc, char* argv[] )
{
	f = fopen(argv[1],"rb");
	if (f)
	{
		Tokenizer tok;
		
		tok.init();
		
		tok.next_token();
			
		fclose(f);
	}
	
	return 0;
}

unsigned char Tokenizer::get_char()
{
	unsigned char c = '\0';
	fread(&c,sizeof(c),1,f);
	return c;
}
