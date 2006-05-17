#include "./OOCore.h"

#include <ace/Log_Msg.h>
#include <ace/SString.h>
#include <ace/OS.h>
#include <ace/Process.h>

OOCore_Export int 
OOCore::ExecProcess(ACE_CString strExeName)
{
	// Set the process options
	ACE_Process_Options options;
	options.avoid_zombies(0);
	options.handle_inheritence(0);
	if (options.command_line(strExeName.c_str()) == -1)
		return -1;

	// Set the creation flags
	u_long flags = 0;
#if defined (ACE_WIN32)
	flags |= CREATE_NEW_CONSOLE;
#endif
	options.creation_flags(flags);

	// Spawn the process
	ACE_Process process;
	if (process.spawn(options)==ACE_INVALID_PID)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to spawn server process\n")),-1);

	// Wait 1 second for the process to launch, if it takes more than 1 second its probably okay
	ACE_exitcode exitcode = 0;
	int ret = process.wait(ACE_Time_Value(1),&exitcode);
	if (ret==-1)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed retrieve process exit code\n")),-1);

	if (ret!=0)
		ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Process exited with code %d.\n"),exitcode),-1);

    return 0;
}
