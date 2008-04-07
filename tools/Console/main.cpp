#include "ConsoleTools.h"
#include "ConsoleApp.h"
#include "Parser.h"

int
main(int argc, char *argv[])
{
	/* run ConsoleTools with the Parser applet */
	ConsoleApp<ConsoleTools<Parser> > app;
	return app.run(argc,argv);
}
