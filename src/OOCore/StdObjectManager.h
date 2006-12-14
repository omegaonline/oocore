#ifndef OOCORE_OBJECT_MANAGER_H_INCLUDED_
#define OOCORE_OBJECT_MANAGER_H_INCLUDED_

class StdObjectManager :
	public OTL::ObjectBase,
	public OTL::AutoObjectFactoryNoAggregation<StdObjectManager,&Omega::OID_StdObjectManager>,
	public Omega::Remoting::IObjectManager,
	public Omega::Remoting::IChannelSink
{
public:
	StdObjectManager();
	virtual ~StdObjectManager();

	BEGIN_INTERFACE_MAP(StdObjectManager)
		INTERFACE_ENTRY(Omega::Remoting::IObjectManager)
		INTERFACE_ENTRY(Omega::Remoting::IChannelSink)
	END_INTERFACE_MAP()

public:
	// Public interface
	Omega::Serialize::IFormattedStream* PrepareRequest(Omega::Remoting::MethodAttributes_t flags);
	Omega::Serialize::IFormattedStream* SendAndReceive(Omega::uint32_t* timeout, Omega::Serialize::IFormattedStream* pToSend);

private:
	struct response_wait
	{
		response_wait(StdObjectManager* t, Omega::uint32_t id, OTL::ObjectPtr<Omega::Serialize::IFormattedStream>& i) : 
			pThis(t), trans_id(id), input(i), except(false)
		{}

		StdObjectManager*									pThis;
		Omega::uint32_t										trans_id;
		OTL::ObjectPtr<Omega::Serialize::IFormattedStream>&	input;
		bool												except;

	private:
		response_wait& operator = (const response_wait&)
		{}
	};
	enum OPCODE
	{
		RESPONSE = 0,
		REQUEST = 1,
		EXCEPTION = 2
	};

	ACE_Recursive_Thread_Mutex m_lock;
	OTL::ObjectPtr<Omega::Activation::IApartment> m_ptrApartment;
	std::vector<OTL::ObjectPtr<Omega::Remoting::IChannel> >	m_vecChannels;
	std::map<Omega::uint32_t,std::pair<OTL::ObjectPtr<Omega::Serialize::IFormattedStream>,bool> > m_response_map;
	Omega::uint32_t m_next_trans_id;
	std::set<Omega::uint32_t> m_transaction_set;

	void process_request(Omega::Serialize::IFormattedStream* input, Omega::uint32_t cookie);
	void process_response(Omega::Serialize::IFormattedStream* input, bool bExcept);
	
	static bool await_response(void* p);
	bool await_response_i(Omega::uint32_t trans_id, OTL::ObjectPtr<Omega::Serialize::IFormattedStream>& input, bool& bExcep);

// IChannelSink
public:
	void OnReceiveMessage(Omega::Serialize::IFormattedStream* pStream, Omega::uint32_t cookie);
	void OnDisconnect(Omega::uint32_t cookie);
	
// IObjectManager members
public:
	void Attach(Omega::Remoting::IChannel* pChannel);
	Omega::IObject* PrepareStaticInterface(const Omega::guid_t& oid, const Omega::guid_t& iid);	
};

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_