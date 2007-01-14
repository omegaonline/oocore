#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

/*namespace {

class Proxy : 
	public ObjectBase,
	public Remoting::IProxyManager
{
public:
	Proxy()
	{ }

	virtual ~Proxy()
	{
		for (std::map<const guid_t,IObject*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
		{
			if (i->second)
				i->second->Release();
		}
	}
	
	void Init(ObjectImpl<StdObjectManager>* pOM)
	{
		m_ptrOM = pOM;
	}

	BEGIN_INTERFACE_MAP(Proxy)
		INTERFACE_ENTRY(Remoting::IProxyManager)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI,0)
	END_INTERFACE_MAP()

// IProxyManager
public:
	virtual Serialize::IFormattedStream* PrepareRequest(Remoting::MethodAttributes_t flags)
	{
		ObjectPtr<Serialize::IFormattedStream> ptrStream;
		ptrStream.Attach(m_ptrOM->PrepareRequest(flags));

		return ptrStream.AddRefReturn();
	}

	Serialize::IFormattedStream* SendAndReceive(uint32_t* timeout, Serialize::IFormattedStream* pToSend)
	{
		return m_ptrOM->SendAndReceive(timeout,pToSend);
	}

protected:
	ObjectPtr<ObjectImpl<StdObjectManager> > m_ptrOM;

private:
	IObject* QI(const guid_t& iid, void*)
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

		std::map<const guid_t,IObject*>::iterator i=m_iid_map.find(iid);
		if (i==m_iid_map.end())
			return 0;

		i->second->AddRef();
		return i->second;
	}

	ACE_Recursive_Thread_Mutex m_lock;
	std::map<const guid_t,IObject*> m_iid_map;
};

class StaticProxy : 
	public Proxy
{
public:
	StaticProxy() : Proxy()
	{}

	void Init(ObjectImpl<StdObjectManager>* pOM, const guid_t& oid)
	{
		Proxy::Init(pOM);
		m_oid = oid;
	}

// IProxyManager
public:
	Serialize::IFormattedStream* PrepareRequest(Remoting::MethodAttributes_t flags)
	{
		ObjectPtr<Serialize::IFormattedStream> ptrStream;
		ptrStream.Attach(m_ptrOM->PrepareRequest(flags));

		// Write the oid...
		MetaInfo::wire_write(this,ptrStream,m_oid);

		return ptrStream.AddRefReturn();
	}

private:
	StaticProxy(const StaticProxy&) {}
	StaticProxy& operator = (const StaticProxy&) {}

	guid_t m_oid;
};

// This is simply for testing message type
const guid_t IID_Message = { 0xce13ec45, 0x65a2, 0x4ae7, { 0x8e, 0xcc, 0xb1, 0x9e, 0xb8, 0xd6, 0xa2, 0xd1 } };

class Message : 
	public ObjectBase,
	public Serialize::IFormattedStream
{
public:
	static ObjectPtr<ObjectImpl<Message> > Create(Remoting::IChannel* pChannel, byte_t type, uint32_t trans_id)
	{
		ObjectPtr<ObjectImpl<Message> > ptrMessage = ObjectImpl<Message>::CreateObjectPtr();
		ptrMessage->Init(pChannel,type,trans_id);
		return ptrMessage;
	}

	static ObjectPtr<ObjectImpl<Message> > Create2(Remoting::IChannel* pChannel, byte_t type, uint32_t trans_id, Remoting::MethodAttributes_t flags)
	{
		ObjectPtr<ObjectImpl<Message> > ptrMessage = ObjectImpl<Message>::CreateObjectPtr();
		ptrMessage->Init(pChannel,type,trans_id);
		ptrMessage->Init2(flags);
		return ptrMessage;
	}

	BEGIN_INTERFACE_MAP(Message)
		INTERFACE_ENTRY_IID(IID_Message,Message)
		INTERFACE_ENTRY_AGGREGATE_BLIND(m_ptrStream)
	END_INTERFACE_MAP()

private:
	ObjectPtr<Serialize::IFormattedStream>	m_ptrStream;
	ObjectPtr<Remoting::IChannel>			m_ptrChannel;
	Remoting::MethodAttributes_t			m_attribs;
	uint32_t								m_trans_id;

	void Init(Remoting::IChannel* pChannel, byte_t type, uint32_t trans_id)
	{
		// Create the output
		m_ptrStream.Attach(pChannel->CreateStream(this));
	        
		// Write the type code and transaction id etc
		WriteUInt32(type);
		WriteUInt32(trans_id);	

		m_ptrChannel = pChannel;
		m_trans_id = trans_id;
	}

	void Init2(Remoting::MethodAttributes_t flags)
	{
		// Write the next stuff
		WriteUInt16(flags);	

		m_attribs = flags;
	}

// IStream members
public:
	uint64_t Size() { return m_ptrStream->Size(); }
	void ReadByte(byte_t& val) { m_ptrStream->ReadByte(val); }
	void ReadBytes(byte_t* val, uint32_t cbBytes) { m_ptrStream->ReadBytes(val,cbBytes); }
	void WriteByte(byte_t val) { m_ptrStream->WriteByte(val); }
	void WriteBytes(const byte_t* val, uint32_t cbBytes) { m_ptrStream->WriteBytes(val,cbBytes); }

// IFormattedStream members
public:
	void ReadUInt16(uint16_t& val) { m_ptrStream->ReadUInt16(val); }
	void ReadUInt32(uint32_t& val) { m_ptrStream->ReadUInt32(val); }
	void ReadUInt64(uint64_t& val) { m_ptrStream->ReadUInt64(val); }
	void WriteUInt16(uint16_t val) { m_ptrStream->WriteUInt16(val); }
	void WriteUInt32(uint32_t val) { m_ptrStream->WriteUInt32(val); }
	void WriteUInt64(const uint64_t& val) { m_ptrStream->WriteUInt64(val); }

// Public members
public:
	ObjectPtr<Remoting::IChannel> GetChannel()
	{
		return m_ptrChannel;
	}

	Remoting::MethodAttributes_t GetAttributes()
	{
		return m_attribs;
	}

	uint32_t GetTransId()
	{
		return m_trans_id;
	}
};

}

namespace Omega { 
namespace MetaInfo 
{ 
	template<> struct iid_traits<Message> { inline static const guid_t& GetIID() { return IID_Message; } }; 
	template<> struct iid_traits<Message*> { inline static const guid_t& GetIID() { return IID_Message; } }; 
}
}

Serialize::IFormattedStream* StdObjectManager::PrepareRequest(Remoting::MethodAttributes_t flags)
{
	// Put it in the transaction map
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Increment trans_id until we can insert it
	uint32_t trans_id;
	do 
	{
		trans_id = m_next_trans_id++;
	} while (!m_transaction_set.insert(trans_id).second);

	guard.release();

	// Select the correct channel for the flags...
	void* TODO;
	ObjectPtr<Remoting::IChannel> ptrChannel;

	// Create the stream
	return Message::Create2(ptrChannel,REQUEST,trans_id,flags).AddRefReturn();
}

Serialize::IFormattedStream* StdObjectManager::SendAndReceive(uint32_t* timeout, Serialize::IFormattedStream* pToSend)
{
	if (!pToSend)
		OOCORE_THROW_ERRNO(EINVAL);
	
	// QI for Message - This will throw if its invalid
	ObjectPtr<Message> ptrMessage(pToSend);
	
	// Send the message on its own channel
	ptrMessage->GetChannel()->SendMessage(pToSend);
		
	ObjectPtr<Serialize::IFormattedStream> ptrRecv;
	if (ptrMessage->GetAttributes() & Remoting::synchronous)
	{
		response_wait rw(this,ptrMessage->GetTransId(),ptrRecv);
		
		if (rw.except)
		{
			IException* pE = 0;
			void* TODO;
			//MetaInfo::wire_read(ptrRecv,pE);
			throw pE;
		}
	}
	
	return ptrRecv.AddRefReturn();
}

void StdObjectManager::OnReceiveMessage(Serialize::IFormattedStream* pInput, uint32_t cookie)
{
	// Check we have input
	if (!pInput)
		OOCORE_THROW_ERRNO(EINVAL);

	// Read the message ident
	byte_t message;
	pInput->ReadByte(message);
	
	switch (message)
	{
	case REQUEST:
		process_request(pInput,cookie);
		break;

	case RESPONSE:
		process_response(pInput,false);
		break;

	case EXCEPTION:
		process_response(pInput,true);
		break;

	default:
		OOCORE_THROW_ERRNO(EINVAL);
		break;
	}
}

void StdObjectManager::process_request(Serialize::IFormattedStream* pInput, uint32_t cookie)
{
	// Read the transaction key
	uint32_t trans_id;
	pInput->ReadUInt32(trans_id);
	
	// Read the attributes
	Remoting::MethodAttributes_t flags;
	pInput->ReadUInt16(flags);

	// Read the stub key
	uint32_t key;
	pInput->ReadUInt32(key);

	ObjectPtr<Remoting::IStub> ptrStub;
	if (key != 0)
	{
		// Find the stub
		//ptrStub = lookup_wire_stub(key);
	}
	else
	{
		// Create a temporary stub, 'cos we are making a static call...
	}
		
	// Read the method number
	uint32_t method;
	pInput->ReadUInt32(method);

	ObjectPtr<ObjectImpl<Message> > ptrOutput;
	ObjectPtr<Remoting::IChannel> ptrChannel;
		
	if (flags & Remoting::synchronous)
	{
		ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

		// Check the cookie is valid
		if (cookie >= m_vecChannels.size() || !m_vecChannels[cookie])
			OOCORE_THROW_ERRNO(EINVAL);

		// Stash the channel
		ptrChannel = m_vecChannels[cookie];

		guard.release();
				
		// Create the output stream
		ptrOutput = Message::Create(ptrChannel,RESPONSE,trans_id);
	}

	// Invoke the method on the stub
	ObjectPtr<IException> ptrE;
	try
	{
		ptrStub->Invoke(method,pInput,ptrOutput);
	}
	catch (IException* pE)
	{
		ptrE.Attach(pE);
	}

	if (flags & Remoting::synchronous)
	{
		if (ptrE)
		{
			// Recreate the output
			ptrOutput = Message::Create(ptrChannel,EXCEPTION,trans_id);
	        
			// Write the exception
			void* TODO;
			//MetaInfo::wire_write(ptrOutput,ptrE);
		}
		
		// Send the response
		ptrChannel->SendMessage(ptrOutput);
	}
}

void StdObjectManager::process_response(Serialize::IFormattedStream* pInput, bool bExcept)
{
	// Read the transaction key
	uint32_t trans_id;
	pInput->ReadUInt32(trans_id);
		
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	try
	{
		// Check if its a transaction we care about
		if (m_transaction_set.find(trans_id) != m_transaction_set.end())
		{
			// Pop it in the arrived map
			if (!m_response_map.insert(std::map<uint32_t,std::pair<ObjectPtr<Serialize::IFormattedStream>,bool> >::value_type(trans_id,std::pair<ObjectPtr<Serialize::IFormattedStream>,bool>(pInput,bExcept))).second)
				IException::Throw("Duplicate transaction id received!",OMEGA_FUNCNAME);
		}
	}
	catch (std::exception& e)
	{
		// Something went wrong with the stl stuff...
		IException::Throw(e.what(),OMEGA_FUNCNAME);
	}
}

bool StdObjectManager::await_response(void* p)
{
	response_wait* rw = static_cast<response_wait*>(p);

	return rw->pThis->await_response_i(rw->trans_id,rw->input,rw->except);
}

bool StdObjectManager::await_response_i(uint32_t trans_id, ObjectPtr<Serialize::IFormattedStream>& input, bool& bExcep)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock,0);

	if (!guard.locked())
		return false;

	std::map<uint32_t,std::pair<ObjectPtr<Serialize::IFormattedStream>,bool> >::iterator i = m_response_map.find(trans_id);
	if (i!=m_response_map.end())
	{
		input = i->second.first;
		bExcep = i->second.second;
		m_response_map.erase(i);
        return true;
	}

	return false;
}

void StdObjectManager::Attach(Remoting::IChannel* pChannel)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Check we haven't added too many already
	if (m_vecChannels.size() > 0x7FFF)
		OOCORE_THROW_ERRNO(EMFILE);

	uint32_t cookie = 0;
	try
	{
		cookie = static_cast<uint32_t>(m_vecChannels.size());
		m_vecChannels.push_back(pChannel);

		pChannel->Attach(0,this,cookie);
	}
	catch (std::exception& e)
	{
		// Something went wrong with the vector stuff...
		IException::Throw(e.what(),OMEGA_FUNCNAME);
	}
	catch (IException* pE)
	{
		// Remove the channel from the vector if we failed to Attach it
		m_vecChannels[cookie].Release();
		throw pE;
	}
}

void StdObjectManager::OnDisconnect(uint32_t cookie)
{
	ACE_Guard<ACE_Recursive_Thread_Mutex> guard(m_lock);

	// Check the cookie is valid
	if (cookie >= m_vecChannels.size() || !m_vecChannels[cookie])
		OOCORE_THROW_ERRNO(EINVAL);

	// There is definitely more to do here....
	void* TODO;

	// Clear the cookie's entry
	m_vecChannels[cookie].Release();
}

IObject* StdObjectManager::PrepareStaticInterface(const guid_t& oid, const guid_t& iid)
{
	ObjectPtr<ObjectImpl<StaticProxy> > ptrProxy = ObjectImpl<StaticProxy>::CreateObjectPtr();

	// Init to us...
	ptrProxy->Init(static_cast<ObjectImpl<StdObjectManager>*>(this),oid);

	return ptrProxy->QueryInterface(iid);
}
*/

// StdObjectManager
StdObjectManager::StdObjectManager() :
	m_uNextStubId(1)
{
}

StdObjectManager::~StdObjectManager()
{
}

void StdObjectManager::Connect(Omega::Remoting::IChannel* pChannel)
{
	if (m_ptrChannel)
		OOCORE_THROW_ERRNO(EALREADY);

	ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	m_ptrChannel = pChannel;
}

void StdObjectManager::Invoke(Omega::Serialize::IFormattedStream* pParamsIn, Omega::Serialize::IFormattedStream* pParamsOut, Omega::uint32_t timeout)
{
	if (!pParamsIn)
		OOCORE_THROW_ERRNO(EINVAL);

	// Process an incoming request...
	try
	{
		ObjectPtr<Omega::MetaInfo::IWireStub> ptrStub;
		Omega::uint32_t method_id;
		
		// Read the stub id and method id
		Omega::uint32_t stub_id = pParamsIn->ReadUInt32();
		if (stub_id == 0)
		{
			// It's a static interface call...
			// N.B. This will always use standard marshalling

			// Read the oid and iid
			guid_t oid = pParamsIn->ReadGuid();
			guid_t iid = pParamsIn->ReadGuid();
			method_id = pParamsIn->ReadUInt32();

			// IObject interface calls are not allowed on static interfaces!
			if (method_id < 3)
				OOCORE_THROW_ERRNO(EINVAL);
					
			// Create the required object
			ObjectPtr<IObject> ptrObject;

			// *** TODO ***  Actually get it out of the ROT
			ptrObject.Attach(Activation::CreateObject(oid,Activation::Any,0,iid));

			// Get the handler for the interface
			const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireStub)
				OMEGA_THROW("No handler for interface");
			
			// And create a stub
			ptrStub.Attach(pRtti->pfnCreateWireStub(this,ptrObject,0));
			if (!ptrStub)
				OMEGA_THROW("No remote handler for interface");
		}
		else
		{
			// It's a method call on a stub...

			// Look up the stub
			try
			{
				ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

				std::map<Omega::uint32_t,ObjectPtr<Omega::MetaInfo::IWireStub> >::const_iterator i=m_mapStubIds.find(stub_id);
				if (i==m_mapStubIds.end())
					OMEGA_THROW("Bad stub id");
				ptrStub = i->second;
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e.what());
			}

			// Read the method id
			method_id = pParamsIn->ReadUInt32();
		}

		// Assume we succeed...
		pParamsOut->WriteBoolean(true);

		void* TODO; // decrease the wait time...

		// Ask the stub to make the call
		ptrStub->Invoke(method_id,pParamsIn,pParamsOut,timeout);
	}
	catch (Omega::IException* pE)
	{
		// Make sure we release the exception
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE);

		// Reply with an exception if we can send replies...
		if (pParamsOut)
		{
			// Dump the previous output...
			pParamsOut->Release();

			// And create a fresh output
			pParamsOut = m_ptrChannel->CreateOutputStream();

			// Mark that we have failed
			pParamsOut->WriteBoolean(false);

			// Write the exception onto the wire
			MetaInfo::wire_write(this,pParamsOut,pE);
		}
	}
}

void StdObjectManager::Disconnect()
{
	ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	// clear the stub map
	m_mapStubIds.clear();
}

void StdObjectManager::MarshalInterface(Serialize::IFormattedStream* pStream, IObject* pObject, const guid_t& iid)
{
	// See if pObject does custom marshalling...
	ObjectPtr<Remoting::IMarshal> ptrMarshal(pObject);
	if (ptrMarshal)
	{
		guid_t oid = ptrMarshal->GetUnmarshalOID(iid,0);
		if (oid != guid_t::NIL)
		{
			// Write the marshalling oid and iid
			pStream->WriteGuid(oid);
			pStream->WriteGuid(iid);

			// Let the custom handle marshalling...
			ptrMarshal->MarshalInterface(pStream,iid,0);

			// Done
			return;
		}
	}
	
	// Get the handler for the interface
	const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
	if (!pRtti || !pRtti->pfnCreateWireStub)
		OMEGA_THROW("No handler for interface");

	// Generate a new key and stub pair
	ObjectPtr<Omega::MetaInfo::IWireStub> ptrStub;
	Omega::uint32_t uId = 0;
	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

		uId = m_uNextStubId++;
		while (uId<1 || m_mapStubIds.find(uId)!=m_mapStubIds.end())
		{
			uId = m_uNextStubId++;
		}

		// Create a stub
		ptrStub.Attach(pRtti->pfnCreateWireStub(this,pObject,uId));
		if (!ptrStub)
			OMEGA_THROW("No remote handler for interface");

		// Add to the map...
		m_mapStubIds.insert(std::map<Omega::uint32_t,ObjectPtr<Omega::MetaInfo::IWireStub> >::value_type(uId,ptrStub));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	// Write out the data
	pStream->WriteGuid(guid_t::NIL);
	pStream->WriteGuid(iid);
	pStream->WriteUInt32(uId);
}

void StdObjectManager::UnmarshalInterface(Omega::Serialize::IFormattedStream* /*pStream*/, const Omega::guid_t& /*iid*/, Omega::IObject*& /*pObject*/)
{
	void* TODO;
}

void StdObjectManager::ReleaseStub(Omega::uint32_t /*id*/)
{
	void* TODO;
}
