///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#include <sys/types.h>
#include <sys/wait.h>

#include "../OOBase/CmdArgs.h"

#include "../include/Omega/version.h"

#define READ_END 0
#define WRITE_END 1

#define STR_LE(n)  #n
#define STR_LE_1 STR_LE(LIBEXEC_DIR)

static int Version()
{
	std::cout << "oo-launch " << OOCORE_VERSION;
#if defined(OMEGA_DEBUG)
	std::cout << " (Debug build)";
#endif
	std::cout << std::endl;

	return EXIT_SUCCESS;
}

static int Help()
{
	std::cout << "oo-launch - The Omega Online user session launch utility." << std::endl;
	std::cout << std::endl;
	std::cout << "Please consult the documentation at http://www.omegaonline.org.uk for further information." << std::endl;
	std::cout << std::endl;
	std::cout << "Usage: oo-launch [options]" << std::endl;
	std::cout << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  --help (-h)      Display this help text" << std::endl;
	std::cout << "  --version (-v)   Display version information" << std::endl;
	std::cout << std::endl;
	std::cout << "  --conf-file (-f) <file_path> Use the specified configuration file" << std::endl;

	return EXIT_SUCCESS;
}

static void do_waitpid(pid_t pid)
{
	while (waitpid(pid,NULL,0) != 0 && errno == EINTR)
		;
}

static void do_exec(const char* path, int fd)
{
	std::ostringstream os;
	os.imbue(std::locale::classic());
	os << "--launch-session=" << fd;

	char* env[] =
	{
		strdup(path),
		strdup(os.str().c_str()),
		0
	};

	execv(path,env);
	std::cerr << "Failed to exec " << path << " " << os.str() << ": " << strerror(errno) << std::endl;

	for (size_t i=0;i<sizeof(env)/sizeof(env[0]);++i)
		free(env[i]);
}

static pid_t run_oosvruser()
{
	int pipes[2] = { -1, -1 };

	if (pipe(pipes) != 0)
	{
		std::cerr << "pipe() failed in run_oosvruser: " << errno << std::endl;
		return -1;
	}

	pid_t parent_id = fork();
	if (parent_id == -1)
	{
		std::cerr << "fork() failed in run_oosvruser (parent): " << strerror(errno) << std::endl;
		return -1;
	}

	if (parent_id == 0)
	{
		// We are the 'parent'

		// We don't need the read end...
		close(pipes[READ_END]);

		pid_t child_id = fork();
		if (child_id == -1)
		{
			std::cerr << "fork() failed in run_oosvruser (child): " << strerror(errno) << std::endl;
			exit(EXIT_FAILURE);
		}
		else if (child_id > 0)
		{
			// Still the 'parent' and we're done...
			exit(EXIT_SUCCESS);
		}

		const char* run = getenv("OMEGA_USER_BINARY");
		if (run)
			do_exec(run,pipes[WRITE_END]);
		else
		{
			do_exec(LIBEXEC_DIR "/oosvruser",pipes[WRITE_END]);

			// Try current directory
			do_exec("./oosvruser",pipes[WRITE_END]);
		}

		exit(EXIT_FAILURE);
	}

	// We are the grandparent

	// We don't need the write end...
	close(pipes[WRITE_END]);

	// Immediately reap the 'parent'
	do_waitpid(parent_id);

	// Read the pid
	pid_t pid = -1;
	if (read(pipes[READ_END],&pid,sizeof(pid)) != sizeof(pid))
		return -1;

	return pid;
}

int main(int argc, char* argv[])
{
	// Set up the command line args
	OOSvrBase::CmdArgs cmd_args;
	cmd_args.add_option("help",'h');
	cmd_args.add_option("version",'v');
	cmd_args.add_option("sh-syntax");
	cmd_args.add_option("csh-syntax");
	cmd_args.add_option("exit-with-session");

	// Parse command line
	std::map<std::string,std::string> args;
	if (!cmd_args.parse(argc,argv,args))
		return EXIT_FAILURE;

	if (args.find("help") != args.end())
		return Help();

	if (args.find("version") != args.end())
		return Version();

	if (args.find("csh-syntax") != args.end() && args.find("sh-syntax") != args.end())
	{
		std::cerr << "Either --sh-syntax or --csh-syntax allowed" << std::endl;
		return EXIT_FAILURE;
	}

	bool bCsh = false;
	if (args.find("csh-syntax") != args.end())
		bCsh = true;
	else if (args.find("sh-syntax") != args.end())
		bCsh = false;
	else
	{
		// Check last 3 letters of $SHELL
		const char* shell = getenv("SHELL");
		if (shell && !strncmp(shell + strlen(shell) - 3,"csh",3))
			bCsh = true;
	}

	bool bExit = (args.find("exit-with-session") != args.end());

	// Now do the fork and exec dance...
	pid_t pid = run_oosvruser();
	if (pid == -1)
		return EXIT_FAILURE;

	if (bExit)
	{

	}

	return EXIT_SUCCESS;
}

namespace OOBase
{
	// This is the critical failure hook
	void CriticalFailure(const char* msg)
	{
		std::cerr << msg << std::endl << std::endl;
		abort();
	}
}
