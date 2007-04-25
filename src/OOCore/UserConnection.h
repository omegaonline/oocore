#ifndef OOCORE_USER_CONNECTION_H_INCLUDED_
#define OOCORE_USER_CONNECTION_H_INCLUDED_

namespace OOCore
{
	class UserSession;

	class UserConnection : public ACE_Service_Handler
	{		
	public:
		UserConnection(UserSession* pSession);
		virtual ~UserConnection();

		int open(ACE_HANDLE new_handle);
		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);
			
	private:
		UserConnection(const UserConnection&) {}
		UserConnection& operator = (const UserConnection&) { return *this; }

		static const size_t			s_initial_read = 8;

		UserSession*				m_pSession;
		ACE_CDR::ULong				m_read_len;
		ACE_Asynch_Read_Stream		m_reader;

		int read();
	};
}

#endif // OOCORE_USER_CONNECTION_H_INCLUDED_
