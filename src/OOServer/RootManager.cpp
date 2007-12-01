///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
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

/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It will be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary
//
/////////////////////////////////////////////////////////////

#include "./OOServer_Root.h"
#include "./RootManager.h"
#include "./SpawnedProcess.h"
#include "./Protocol.h"

#if defined(ACE_WIN32)
// For the Windows path functions
#include <shlwapi.h>
#include <shlobj.h>
#endif

Root::Manager::Manager() :
	m_sandbox_channel(0)
{
}

Root::Manager::~Manager()
{
}

bool Root::Manager::install(int argc, wchar_t* argv[])
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening registry"),false);

	if (!SpawnedProcess::InstallSandbox(argc,argv))
		return false;

	// Add the default keys
	ACE_Configuration_Section_Key res;
	ROOT_MANAGER::instance()->m_registry.open_section(ROOT_MANAGER::instance()->m_registry.root_section(),L"All Users",1,res);
	ROOT_MANAGER::instance()->m_registry.open_section(ROOT_MANAGER::instance()->m_registry.root_section(),L"Objects",1,res);
	ROOT_MANAGER::instance()->m_registry.open_section(ROOT_MANAGER::instance()->m_registry.root_section(),L"Objects\\OIDs",1,res);

	// Now secure the files we will use...
	if (!SpawnedProcess::SecureFile(ROOT_MANAGER::instance()->m_strRegistry))
		return false;

	return true;
}

bool Root::Manager::uninstall()
{
	if (ROOT_MANAGER::instance()->init_registry() != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%p\n",L"Error opening registry"),false);

	if (!SpawnedProcess::UninstallSandbox())
		return false;

	return true;
}

int Root::Manager::run(int argc, wchar_t* argv[])
{
	return ROOT_MANAGER::instance()->run_event_loop_i(argc,argv);
}

ACE_Configuration_Heap& Root::Manager::get_registry()
{
	return ROOT_MANAGER::instance()->m_registry;
}

void Root::Manager::end()
{
	ROOT_MANAGER::instance()->end_event_loop_i();
}

int Root::Manager::run_event_loop_i(int /*argc*/, wchar_t* /*argv*/[])
{
	int ret = -1;

	// Determine default threads from processor count
	int threads = ACE_OS::num_processors();
	if (threads < 2)
		threads = 2;

	// Spawn off the request threads
	int req_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,request_worker_fn,this);
	if (req_thrd_grp_id == -1)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"spawn() failed"),-1);
	else
	{
		// Spawn off the proactor threads
		int pro_thrd_grp_id = ACE_Thread_Manager::instance()->spawn_n(threads,proactor_worker_fn);
		if (pro_thrd_grp_id == -1)
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"spawn() failed"),-1);
		else
		{
			if (init())
			{
				// Now just process client requests
				ret = process_client_connects();

				// Wait for all the proactor threads to finish
				ACE_Thread_Manager::instance()->wait_grp(pro_thrd_grp_id);

				// Wait for all the request threads to finish
				ACE_Thread_Manager::instance()->wait_grp(req_thrd_grp_id);
			}
		}
	}

	return ret;
}

ACE_THR_FUNC_RETURN Root::Manager::proactor_worker_fn(void*)
{
	return (ACE_THR_FUNC_RETURN)ACE_Proactor::instance()->proactor_run_event_loop();
}

ACE_THR_FUNC_RETURN Root::Manager::request_worker_fn(void* pParam)
{
	static_cast<Manager*>(pParam)->pump_requests();
	return 0;
}

bool Root::Manager::init()
{
	// Open the root registry
	if (init_registry() != 0)
		return false;

	// Spawn the sandbox
	ACE_WString strPipe;
	m_sandbox_channel = spawn_user(static_cast<user_id_type>(0),0,strPipe);
	if (!m_sandbox_channel)
		return false;

	return true;
}

int Root::Manager::init_registry()
{
#define OMEGA_REGISTRY_FILE L"system.regdb"

#if defined(ACE_WIN32)

	m_strRegistry = L"C:\\" OMEGA_REGISTRY_FILE;

	wchar_t szBuf[MAX_PATH] = {0};
	HRESULT hr = SHGetFolderPathW(0,CSIDL_COMMON_APPDATA,0,SHGFP_TYPE_DEFAULT,szBuf);
	if FAILED(hr)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] SHGetFolderPathW failed: %x\n",GetLastError()),-1);
	else
	{
		wchar_t szBuf2[MAX_PATH] = {0};
		if (!PathCombineW(szBuf2,szBuf,L"Omega Online"))
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] PathCombine failed: %x\n",GetLastError()),-1);
		else
		{
			if (!PathFileExistsW(szBuf2))
			{
				int ret = ACE_OS::mkdir(szBuf2);
				if (ret != 0)
					ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"mkdir failed"),-1);
			}

			if (!PathCombineW(szBuf,szBuf2,OMEGA_REGISTRY_FILE))
				ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] PathCombine failed: %x\n",GetLastError()),-1);
			else
				m_strRegistry = szBuf;
		}
	}

#else

	#define OMEGA_REGISTRY_DIR L"/var/lib/omegaonline"

	if (ACE_OS::mkdir(OMEGA_REGISTRY_DIR,S_IRWXU | S_IRWXG | S_IROTH) != 0)
	{
		int err = ACE_OS::last_error();
		if (err != EEXIST)
			ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"mkdir failed"),-1);
	}
	m_strRegistry = OMEGA_REGISTRY_DIR L"/" OMEGA_REGISTRY_FILE;

#endif

	if (m_registry.open(m_strRegistry.c_str(),(char*)0x40000000) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"registry open() failed"),-1);

	return 0;
}

void Root::Manager::end_event_loop_i()
{
	// Stop accepting new clients
	m_client_connector.stop();

	// Stop the reactor
	ACE_Reactor::instance()->end_reactor_event_loop();

	// Close the user processes
	close_users();

	// Stop the proactor
	ACE_Proactor::instance()->end_event_loop();

	// Stop the MessageHandler
	stop();
}

int Root::Manager::process_client_connects()
{
#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	const wchar_t* pipe_name = L"ooserver";
#else
	const wchar_t* pipe_name = L"/var/ooserver";
#endif

	if (m_client_connector.start(this,1,pipe_name) != 0)
		return -1;

	int ret = ACE_Reactor::instance()->run_reactor_event_loop();

	ACE_Reactor::close_singleton();

	return ret;
}

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
int Root::Manager::on_accept(ACE_SPIPE_Stream& pipe, int)
{
#else
int Root::Manager::on_accept(MessagePipe& pipe, int key)
{
	if (key == 0)
		return MessageHandler::on_accept(pipe,key);
#endif

	user_id_type uid = 0;

	// Read the uid - we must read even for Windows
	if (pipe.recv(&uid,sizeof(uid)) != static_cast<ssize_t>(sizeof(uid)))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"recv() failed"),-1);

#if defined(ACE_HAS_WIN32_NAMED_PIPES)
	if (!ImpersonateNamedPipeClient(pipe.get_handle()))
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] ImpersonateNamedPipeClient failed: %x\n",GetLastError()),-1);

	BOOL bRes = OpenThreadToken(GetCurrentThread(),TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_IMPERSONATE,FALSE,&uid);
	if (!RevertToSelf())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] RevertToSelf failed: %x\n",GetLastError()));
		CloseHandle(uid);
		exit(-1);
	}
	if (!bRes)
	{
		CloseHandle(uid);
		return -1;
	}
#endif

	ACE_WString strPipe;
	if (connect_client(uid,strPipe))
	{
		size_t uLen = strPipe.length()+1;
		pipe.send(&uLen,sizeof(uLen));
		pipe.send(strPipe.c_str(),uLen*sizeof(wchar_t));
	}

	pipe.close();
	return 0;
}

ACE_CDR::UShort Root::Manager::spawn_user(user_id_type uid, ACE_CDR::UShort nUserChannel, ACE_WString& strPipe)
{
	ACE_WString strNewPipe = MessagePipe::unique_name(L"oo");

	MessagePipeAcceptor acceptor;
	if (acceptor.open(strNewPipe.c_str(),uid) != 0)
		ACE_ERROR_RETURN((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.open() failed"),0);
	
	// Alloc a new SpawnedProcess
	SpawnedProcess* pSpawn = 0;
	ACE_NEW_RETURN(pSpawn,SpawnedProcess,false);

	ACE_CDR::UShort nChannelId = 0;

	// Spawn the user process
	if (pSpawn->Spawn(uid,strNewPipe))
	{
		// Accept
#ifdef OMEGA_DEBUG
		ACE_Time_Value wait(120);
#else
		ACE_Time_Value wait(30);
#endif
		MessagePipe pipe;
		if (acceptor.accept(pipe,&wait) != 0)
			ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"acceptor.accept() failed"));
		else
		{
			strPipe = bootstrap_user(pipe,nUserChannel);
			if (!strPipe.empty())
			{
				// Create a new MessageConnection
				MessageConnection* pMC = 0;
				ACE_NEW_NORETURN(pMC,MessageConnection(this));
				if (!pMC)
					ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %m\n"));
				else if ((nChannelId = pMC->open(pipe)) != 0)
				{
					// Insert the data into various maps...
					try
					{
						ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

						UserProcess process = {strPipe, pSpawn};
						m_mapUserProcesses.insert(std::map<MessagePipe,UserProcess>::value_type(pipe,process));
					}
					catch (...)
					{
						ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Unhandled exception\n"));
						nChannelId = 0;
					}
				}

				if (!nChannelId)
					delete pMC;
			}
		}
	}

	acceptor.close();
	
	if (!nChannelId)
		delete pSpawn;

	return nChannelId;
}

void Root::Manager::close_users()
{
    // Tell all user processes to close
	ACE_OutputCDR request;
	request << static_cast<RootOpCode_t>(Root::End);
	if (request.good_bit())
	{
		try
		{
			// Iterate backwards
			for (std::map<MessagePipe,UserProcess>::reverse_iterator i=m_mapUserProcesses.rbegin();i!=m_mapUserProcesses.rend();++i)
			{
                ACE_CDR::UShort channel = get_pipe_channel(i->first,0);

				ACE_InputCDR* response = 0;
				send_request(channel,request.begin(),response,1000,1);
			}
		}
		catch (...)
		{
		}
	}

	// Close all connections to user processes
	try
	{
		for (std::map<MessagePipe,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
		{
			delete i->second.pSpawn;
		}
	}
	catch (...)
	{}
}

ACE_WString Root::Manager::bootstrap_user(MessagePipe& pipe, ACE_CDR::UShort nUserChannel)
{
	if (pipe.send(&m_sandbox_channel,sizeof(m_sandbox_channel)) != sizeof(m_sandbox_channel))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"));
		return L"";
	}

	if (pipe.send(&nUserChannel,sizeof(nUserChannel)) != sizeof(nUserChannel))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.send() failed"));
		return L"";
	}

	size_t uLen = 0;
	if (pipe.recv(&uLen,sizeof(uLen)) != static_cast<ssize_t>(sizeof(uLen)))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.recv() failed"));
		return L"";
	}

	wchar_t* buf;
	ACE_NEW_RETURN(buf,wchar_t[uLen],L"");

	if (pipe.recv(buf,uLen*sizeof(wchar_t)) != static_cast<ssize_t>(uLen*sizeof(wchar_t)))
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"pipe.recv() failed"));
		delete [] buf;
		return L"";
	}

	ACE_WString strRet = buf;
	delete [] buf;

	return strRet;
}

bool Root::Manager::connect_client(user_id_type uid, ACE_WString& strPipe)
{
	try
	{
		ACE_CDR::UShort nUserChannel = 0;

		// See if we have a process already
		UserProcess process = {L"",0};
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			for (std::map<MessagePipe,UserProcess>::iterator i=m_mapUserProcesses.begin();i!=m_mapUserProcesses.end();++i)
			{
				if (i->second.pSpawn->Compare(uid))
				{
					process = i->second;
					break;
				}
				else if (i->second.pSpawn->IsSameUser(uid))
				{
					nUserChannel = get_pipe_channel(i->first,0);
				}
			}
		}

		// See if its still running...
		if (process.pSpawn)
		{
			strPipe = process.strPipe;
			return true;
		}

		return spawn_user(uid,nUserChannel,strPipe) != 0;
	}
	catch (std::exception&)
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Unhandled exception\n"));
	}

	return false;
}

void Root::Manager::pipe_closed(const MessagePipe& pipe)
{
	Root::MessageHandler::pipe_closed(pipe);

	try
	{
		ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
	
		std::map<MessagePipe,UserProcess>::iterator i=m_mapUserProcesses.find(pipe);
		if (i != m_mapUserProcesses.end())
		{
			delete i->second.pSpawn;
			m_mapUserProcesses.erase(i);
		}
	}
	catch (...)
	{}
}

bool Root::Manager::access_check(const MessagePipe& pipe, const wchar_t* pszObject, ACE_UINT32 mode, bool& bAllowed)
{
	try
	{
		SpawnedProcess* pSpawn;
		{
			ACE_READ_GUARD_RETURN(ACE_RW_Thread_Mutex,guard,m_lock,false);

			// Find the process info
			std::map<MessagePipe,UserProcess>::iterator i = m_mapUserProcesses.find(pipe);
			if (i == m_mapUserProcesses.end())
			{
				ACE_OS::last_error(EINVAL);
				return false;
			}

			pSpawn = i->second.pSpawn;
		}

		return pSpawn->CheckAccess(pszObject,mode,bAllowed);
	}
	catch (...)
	{
		ACE_OS::last_error(EINVAL);
		return false;
	}
}

void Root::Manager::process_request(const MessagePipe& pipe, ACE_InputCDR& request, ACE_CDR::UShort src_channel_id, ACE_CDR::UShort src_thread_id, const ACE_Time_Value& deadline, ACE_CDR::UShort /*attribs*/)
{
	RootOpCode_t op_code;
	request >> op_code;

	//ACE_DEBUG((LM_DEBUG,L"Root context: Root request %u from %u(%u)",op_code,reply_channel_id,src_channel_id));

	if (!request.good_bit())
	{
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] %p\n",L"Bad request"));
		return;
	}

	ACE_OutputCDR response;

	switch (op_code)
	{
	case KeyExists:
		registry_key_exists(pipe,request,response);
		break;

	case CreateKey:
		registry_create_key(pipe,request,response);
		break;

	case DeleteKey:
		registry_delete_key(pipe,request,response);
		break;

	case EnumSubKeys:
		registry_enum_subkeys(pipe,request,response);
		break;

	case ValueType:
		registry_value_type(pipe,request,response);
		break;

	case GetStringValue:
		registry_get_string_value(pipe,request,response);
		break;

	case GetUInt32Value:
		registry_get_uint_value(pipe,request,response);
		break;

	case GetBinaryValue:
		registry_get_binary_value(pipe,request,response);
		break;

	case SetStringValue:
		registry_set_string_value(pipe,request,response);
		break;

	case SetUInt32Value:
		registry_set_uint_value(pipe,request,response);
		break;

	case SetBinaryValue:
		registry_set_binary_value(pipe,request,response);
		break;

	case EnumValues:
		registry_enum_values(pipe,request,response);
		break;

	case DeleteValue:
		registry_delete_value(pipe,request,response);
		break;

	default:
		response.write_long(EINVAL);
		ACE_ERROR((LM_ERROR,L"%N:%l [%P:%t] Bad request op_code\n"));
		break;
	}

	if (response.good_bit())
		send_response(src_channel_id,src_thread_id,response.begin(),deadline,0);
}

// Annoyingly this is missing from ACE...  A direct lift and translate of the ACE_CString version
static ACE_CDR::Boolean read_wstring(ACE_InputCDR& stream, ACE_WString& x)
{
	ACE_CDR::WChar *data = 0;
	if (stream.read_wstring(data))
	{
		x = data;
		delete [] data;
		return true;
	}

	x = L"";
	return stream.good_bit();
}

bool Root::Manager::registry_open_section(const MessagePipe& pipe, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, bool bAccessCheck)
{
	ACE_WString strKey;
	if (!read_wstring(request,strKey))
		return false;

	if (bAccessCheck)
	{
		bool bAllowed = false;
		if (strKey.substr(0,9) == L"All Users")
			bAllowed = true;
		else if (!access_check(pipe,m_strRegistry.c_str(),O_RDWR,bAllowed))
			return false;

		if (!bAllowed)
		{
			ACE_OS::last_error(EACCES);
			return false;
		}
	}

	if (strKey.length()==0)
		key = m_registry.root_section();
	else
	{
		if (m_registry.open_section(m_registry.root_section(),strKey.c_str(),0,key) != 0)
			return false;
	}

	return true;
}

bool Root::Manager::registry_open_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_Configuration_Section_Key& key, ACE_WString& strValue, bool bAccessCheck)
{
	if (!registry_open_section(pipe,request,key,bAccessCheck))
		return false;

	if (!read_wstring(request,strValue))
		return false;

	return true;
}

void Root::Manager::registry_key_exists(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::Boolean bRes = false;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(pipe,request,key))
			{
				if (ACE_OS::last_error() != ENOENT)
					err = ACE_OS::last_error();
			}
			else
			{
				bRes = true;
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_boolean(bRes);
}

void Root::Manager::registry_create_key(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_WString strKey;
	if (!read_wstring(request,strKey))
		err = ACE_OS::last_error();
	else
	{
        bool bAllowed = false;
		if (strKey.substr(0,9) == L"All Users")
			bAllowed = true;
		else if (!access_check(pipe,m_strRegistry.c_str(),O_RDWR,bAllowed))
			err = ACE_OS::last_error();

		if (!bAllowed)
			err = EACCES;
		else
		{
			ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
			if (guard.locked () == 0)
				err = ACE_OS::last_error();
			else
			{
				ACE_Configuration_Section_Key key;
				if (m_registry.open_section(m_registry.root_section(),strKey.c_str(),1,key) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_delete_key(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
    int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(pipe,request,key,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_WString strSubKey;
				if (!read_wstring(request,strSubKey))
					err = ACE_OS::last_error();
				else
				{
					if (strSubKey == L"All Users" ||
						strSubKey == L"Objects" ||
						strSubKey == L"Objects\\OIDs" ||
						strSubKey == L"Server" ||
						strSubKey == L"Server\\Sandbox")
					{
						err = EACCES;
					}
					else if (m_registry.remove_section(key,strSubKey.c_str(),1) != 0)
						err = ACE_OS::last_error();
				}
			}
		}
	}

	response << err;
}

void Root::Manager::registry_enum_subkeys(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	std::list<ACE_WString> listSections;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(pipe,request,key))
				err = ACE_OS::last_error();
			else
			{
				try
				{
					for (int index=0;;++index)
					{
						ACE_WString strSubKey;
						int e = m_registry.enumerate_sections(key,index,strSubKey);
						if (e == 0)
							listSections.push_back(strSubKey);
						else
						{
							if (e != 1)
								err = ACE_OS::last_error();
							break;
						}
					}
				}
				catch (std::exception&)
				{
					err = EINVAL;
				}
			}
		}
	}

	response << err;
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listSections.size());
			for (std::list<ACE_WString>::iterator i=listSections.begin();i!=listSections.end();++i)
			{
				response.write_wstring(i->c_str());
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_value_type(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::Octet type = 0;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				ACE_Configuration_Heap::VALUETYPE vtype;
				if (m_registry.find_value(key,strValue.c_str(),vtype) == 0)
					type = static_cast<ACE_CDR::Octet>(vtype);
				else
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_octet(type);
}

void Root::Manager::registry_get_string_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_WString strText;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				if (m_registry.get_string_value(key,strValue.c_str(),strText) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_wstring(strText.c_str());
}

void Root::Manager::registry_get_uint_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::ULong val = 0;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				if (m_registry.get_integer_value(key,strValue.c_str(),val) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
	if (err == 0)
		response.write_ulong(val);
}

void Root::Manager::registry_get_binary_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	ACE_CDR::ULong len = 0;
	void* data = 0;
	bool bReplyWithData = false;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue))
				err = ACE_OS::last_error();
			else
			{
				if (!request.read_ulong(len))
					err = ACE_OS::last_error();
				else
				{
					bReplyWithData = (len != 0);
					size_t dlen = 0;
					if (m_registry.get_binary_value(key,strValue.c_str(),data,dlen) != 0)
						err = ACE_OS::last_error();
					else if (len != 0)
						len = std::min(len,static_cast<ACE_CDR::ULong>(dlen));
					else
						len = static_cast<ACE_CDR::ULong>(dlen);
				}
			}
		}
	}

	response << err;
	if (err == 0)
	{
		response.write_ulong(len);
		if (bReplyWithData)
			response.write_octet_array(static_cast<const ACE_CDR::Octet*>(data),len);
	}

	delete [] static_cast<char*>(data);
}

void Root::Manager::registry_set_string_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_WString strText;
				if (!read_wstring(request,strText))
					err = ACE_OS::last_error();
				else if (m_registry.set_string_value(key,strValue.c_str(),strText) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_set_uint_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_CDR::ULong iValue;
				if (!request.read_ulong(iValue))
					err = ACE_OS::last_error();
				else if (m_registry.set_integer_value(key,strValue.c_str(),iValue) != 0)
					err = ACE_OS::last_error();
			}
		}
	}

	response << err;
}

void Root::Manager::registry_set_binary_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue,true))
				err = ACE_OS::last_error();
			else
			{
				ACE_CDR::ULong len;
				if (!request.read_ulong(len))
					err = ACE_OS::last_error();
				else
				{
					// TODO - This could be made quicker by aligning the read_ptr and not copying... but not today...
					ACE_CDR::Octet* data = 0;
					ACE_NEW_NORETURN(data,ACE_CDR::Octet[len]);
					if (!data)
						err = ENOMEM;
					else
					{
						if (!request.read_octet_array(data,len))
							err = ACE_OS::last_error();
						else if (m_registry.set_binary_value(key,strValue.c_str(),data,len) != 0)
							err = ACE_OS::last_error();

						delete [] data;
					}
				}
			}
		}
	}

	response << err;
}

void Root::Manager::registry_enum_values(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	std::list<ACE_WString> listValues;
	{
		ACE_Read_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			if (!registry_open_section(pipe,request,key))
				err = ACE_OS::last_error();
			else
			{
				try
				{
					for (int index=0;;++index)
					{
						ACE_WString strValue;
						ACE_Configuration_Heap::VALUETYPE type;
						int e = m_registry.enumerate_values(key,index,strValue,type);
						if (e == 0)
							listValues.push_back(strValue);
						else
						{
							if (e != 1)
								err = ACE_OS::last_error();
							break;
						}
					}
				}
				catch (std::exception&)
				{
					err = EINVAL;
				}
			}
		}
	}

	response << err;
	if (err == 0)
	{
		try
		{
			response.write_ulonglong(listValues.size());
			for (std::list<ACE_WString>::iterator i=listValues.begin();i!=listValues.end();++i)
			{
				response.write_wstring(i->c_str());
			}
		}
		catch (std::exception&)
		{}
	}
}

void Root::Manager::registry_delete_value(const MessagePipe& pipe, ACE_InputCDR& request, ACE_OutputCDR& response)
{
	int err = 0;
	{
		ACE_Write_Guard<ACE_RW_Thread_Mutex> guard(m_registry_lock);
		if (guard.locked () == 0)
			err = ACE_OS::last_error();
		else
		{
			ACE_Configuration_Section_Key key;
			ACE_WString strValue;
			if (!registry_open_value(pipe,request,key,strValue,true))
				err = ACE_OS::last_error();
			else if (m_registry.remove_value(key,strValue.c_str()) != 0)
				err = ACE_OS::last_error();
		}
	}

	response << err;
}

