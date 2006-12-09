#ifndef OOCORE_USER_SERVICE_H_INCLUDED_
#define OOCORE_USER_SERVICE_H_INCLUDED_

#define ACE_HAS_VERSIONED_NAMESPACE  1
#define ACE_AS_STATIC_LIBS

#include <ace/Acceptor.h>
#include <ace/Connector.h>
#include <ace/SOCK_Stream.h>
#include <ace/Auto_Ptr.h>
#include <ace/CDR_Stream.h>
#include <ace/Message_Block.h>
#include <ace/Method_Request.h>
#include <ace/Svc_Handler.h>

#include <OOCore/Remoting.h>
#include <OTL/OTL.h>

namespace Session
{
	OMEGA_DECLARE_IID(OutputCDR);

	class OutputCDR :
		public OTL::ObjectBase,
		public ACE_OutputCDR,
		public Omega::Serialize::IFormattedStream
	{
	public:
		BEGIN_INTERFACE_MAP(OutputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
			INTERFACE_ENTRY_IID(IID_OutputCDR,OutputCDR)
		END_INTERFACE_MAP()

	private:
		void no_access()
		{
			OMEGA_THROW(ACE_OS::strerror(EACCES));
		}

		void throw_errno()
		{
			OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
		}

	// IStream members
	public:
		Omega::uint64_t Size() 
			{ return total_length(); }
		void ReadByte(Omega::byte_t&) 
			{ no_access(); }
		void ReadBytes(Omega::byte_t*, Omega::uint32_t) 
			{ no_access(); }
		void WriteByte(Omega::byte_t val) 
			{ if (!write_octet(val)) throw_errno(); }
		void WriteBytes(const Omega::byte_t* val, Omega::uint32_t cbBytes) 
			{ if (!write_octet_array(val,cbBytes)) throw_errno(); }

	// IFormattedStream members
	public:
		void ReadUInt16(Omega::uint16_t&) 
			{ no_access(); }
		void ReadUInt32(Omega::uint32_t&) 
			{ no_access(); }
		void ReadUInt64(Omega::uint64_t&) 
			{ no_access(); }
		void WriteUInt16(Omega::uint16_t val)
			{ if (!write_ushort(val)) throw_errno(); }
		void WriteUInt32(Omega::uint32_t val)
			{ if (!write_ulong(val)) throw_errno(); }
		void WriteUInt64(const Omega::uint64_t& val)
			{ if (!write_ulonglong(val)) throw_errno(); }
	};

	OMEGA_DECLARE_IID(InputCDR);

	class InputCDR :
		public OTL::ObjectBase,
		public ACE_InputCDR,
		public Omega::Serialize::IFormattedStream
	{
	public:
		InputCDR() : ACE_InputCDR(size_t(0))
		{}

		void Init(const ACE_InputCDR& i)
		{
			*static_cast<ACE_InputCDR*>(this) = i;
		}

		BEGIN_INTERFACE_MAP(InputCDR)
			INTERFACE_ENTRY(Omega::Serialize::IFormattedStream)
			INTERFACE_ENTRY(Omega::Serialize::IStream)
			INTERFACE_ENTRY_IID(IID_InputCDR,InputCDR)
		END_INTERFACE_MAP()

	private:
		void no_access()
		{
			OMEGA_THROW(ACE_OS::strerror(EACCES));
		}

		void throw_errno()
		{
			OMEGA_THROW(ACE_OS::strerror(ACE_OS::last_error()));
		}

	// IStream members
	public:
		Omega::uint64_t Size() 
			{ return length(); }
		void ReadByte(Omega::byte_t& val)
			{ if (!read_octet(val)) throw_errno(); }
		void ReadBytes(Omega::byte_t* val, Omega::uint32_t cbBytes)
			{ if (!read_octet_array(val,cbBytes)) throw_errno(); }
		void WriteByte(Omega::byte_t) 
			{ no_access(); }
		void WriteBytes(const Omega::byte_t*, Omega::uint32_t) 
			{ no_access(); }

	// IFormattedStream members
	public:
		void ReadUInt16(Omega::uint16_t& val)
			{ if (!read_ushort(val)) throw_errno(); }
		void ReadUInt32(Omega::uint32_t& val)
			{ if (!read_ulong(val)) throw_errno(); }
		void ReadUInt64(Omega::uint64_t& val)
			{ if (!read_ulonglong(val)) throw_errno(); }
		void WriteUInt16(Omega::uint16_t) 
			{ no_access(); }
		void WriteUInt32(Omega::uint32_t) 
			{ no_access(); }
		void WriteUInt64(const Omega::uint64_t&) 
			{ no_access(); }
	};

	template <class SVC_HANDLER, ACE_PEER_CONNECTOR_1>
	class Connector : public ACE_Connector<SVC_HANDLER, ACE_PEER_CONNECTOR_2>
	{
	public:
		Connector(ACE_Reactor *r = ACE_Reactor::instance(), int flags = 0) : ACE_Connector<SVC_HANDLER, ACE_PEER_CONNECTOR_2>(r,flags)
		{ }

	protected:
		int make_svc_handler(SVC_HANDLER*& sh)
		{
			ACE_TRACE ("Connector<SVC_HANDLER, ACE_PEER_CONNECTOR_2>::make_svc_handler");

			if (sh == 0)
			{
				try
				{
					sh = SVC_HANDLER::CreateObject();
				}
				catch (Omega::IException* pE)
				{
					pE->Release();
					return -1;
				}
			}

			// Set the reactor of the newly created <SVC_HANDLER> to the same
			// reactor that this <ACE_Connector> is using.
			sh->reactor (this->reactor());
			return 0;
		}

	private:
		Connector(const Connector&) {}
		Connector& operator = (const Connector&) {}
	};

	template <class SVC_HANDLER, ACE_PEER_ACCEPTOR_1>
	class Acceptor : public ACE_Acceptor<SVC_HANDLER, ACE_PEER_ACCEPTOR_2>
	{
	protected:
		int make_svc_handler(SVC_HANDLER*& sh)
		{
			ACE_TRACE ("Acceptor<SVC_HANDLER, ACE_PEER_ACCEPTOR_2>::make_svc_handler");

			if (sh == 0)
			{
				try
				{
					sh = SVC_HANDLER::CreateObject();
				}
				catch (Omega::IException* pE)
				{
					pE->Release();
					return -1;
				}
			}

			// Set the reactor of the newly created <SVC_HANDLER> to the same
			// reactor that this <ACE_Acceptor> is using.
			sh->reactor (this->reactor());
			return 0;
		}
	};
}

class UserService :
	public ACE_Svc_Handler<ACE_SOCK_Stream, ACE_MT_SYNCH>,
	public OTL::ObjectBase,
	public Omega::Remoting::IChannel
{
	typedef ACE_Svc_Handler<ACE_SOCK_Stream, ACE_MT_SYNCH> svc_class;

BEGIN_INTERFACE_MAP(UserService)
	INTERFACE_ENTRY(Omega::Remoting::IChannel)
END_INTERFACE_MAP()

protected:
	UserService()  :
		m_curr_block(0)
	{}

public:
	int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int handle_output(ACE_HANDLE fd = ACE_INVALID_HANDLE);
	virtual int handle_close(ACE_HANDLE fd = ACE_INVALID_HANDLE, ACE_Reactor_Mask mask = ACE_Event_Handler::ALL_EVENTS_MASK);

protected:
	virtual void send(ACE_Message_Block* mb, ACE_Time_Value* wait = 0);
	virtual int recv(ACE_Message_Block*& mb, ACE_Time_Value* wait = 0);

private:
	ACE_Recursive_Thread_Mutex		m_lock;
	ACE_Message_Block*				m_curr_block;

	Omega::uint32_t										m_cookie;
	OTL::ObjectPtr<Omega::Remoting::IChannelSink>		m_ptrSink;

	struct msg_param
	{
		Omega::uint32_t											cookie;
		OTL::ObjectPtr<Omega::Remoting::IChannelSink>			ptrSink;
		OTL::ObjectPtr<OTL::ObjectImpl<Session::InputCDR> >		ptrInput;
	};

	static void process_message(void* param);	
	int read_header(ACE_InputCDR& input, size_t& msg_size);
	int send_i();

	Omega::guid_t next_filter_oid();

// Omega::Remoting::IChannel memebers
public:
	virtual void Attach(Omega::Remoting::IChannel* pOverlay, Omega::Remoting::IChannelSink* pSink, Omega::uint32_t cookie);
	virtual void Detach();
	virtual Omega::Serialize::IFormattedStream* CreateStream(Omega::IObject* pOuter);
	virtual void SendMessage(Omega::Serialize::IFormattedStream* pStream);
};

#endif // OOCORE_USER_SERVICE_H_INCLUDED_