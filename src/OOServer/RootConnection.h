/////////////////////////////////////////////////////////////
//
//	***** THIS IS A SECURE MODULE *****
//
//	It can be run as Administrator/setuid root
//
//	Therefore it needs to be SAFE AS HOUSES!
//
//	Do not include anything unecessary and do not use precompiled headers
//
/////////////////////////////////////////////////////////////

#ifndef OOSERVER_ROOT_CONNECTION_H_INCLUDED_
#define OOSERVER_ROOT_CONNECTION_H_INCLUDED_

class HandlerBase;

namespace Root
{
	class Connection : public ACE_Service_Handler
	{
	public:
		Connection(HandlerBase* pBase, const ACE_CString& key);
		virtual ~Connection();

		bool open(ACE_HANDLE new_handle);
		void handle_read_stream(const ACE_Asynch_Read_Stream::Result& result);

	private:
		Connection(const Connection&) {}
		Connection& operator = (const Connection&) { return *this; }

		static const size_t         s_initial_read = 8;
		HandlerBase*                m_pBase;
		ACE_CString                 m_id;
		ACE_CDR::ULong              m_read_len;
		ACE_Asynch_Read_Stream      m_reader;

		bool read();
	};
}

#endif // OOSERVER_ROOT_CONNECTION_H_INCLUDED_
