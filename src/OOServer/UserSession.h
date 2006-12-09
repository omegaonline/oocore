#ifndef OOSERVER_USER_SESSION_H_INCLUDED_
#define OOSERVER_USER_SESSION_H_INCLUDED_

int UserMain(u_short uPort);
int StartReactor();

class UserSession : public ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>
{		
public:
	UserSession() : ACE_Svc_Handler<ACE_SOCK_STREAM, ACE_MT_SYNCH>()
	{}

	int open(void* p = 0);
	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int handle_close(ACE_HANDLE fd, ACE_Reactor_Mask mask);	

private:
	UserSession(const UserSession&) {}
	UserSession& operator = (const UserSession&) {}
};

#endif // OOSERVER_USER_SESSION_H_INCLUDED_