#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

namespace 
{
	class UnboundProxy : 
		public ObjectBase,
		public IObject
	{
	public:
		void init(Remoting::IWireManager* pManager, const guid_t& oid)
		{
			m_ptrManager = pManager;
			m_oid = oid;
		}

	BEGIN_INTERFACE_MAP(UnboundProxy)
		INTERFACE_ENTRY(IObject)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI,0)
	END_INTERFACE_MAP()

	private:
		guid_t                            m_oid;
		ObjectPtr<Remoting::IWireManager> m_ptrManager;

		inline IObject* QI(const guid_t& iid, void*);
	};
}

StdObjectManager::StdObjectManager() :
	m_uNextStubId(1)
{
}

StdObjectManager::~StdObjectManager()
{
}

void StdObjectManager::Connect(Remoting::IChannel* pChannel)
{
	if (m_ptrChannel)
		OOCORE_THROW_ERRNO(EALREADY);

	ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	m_ptrChannel = pChannel;
}

void StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout)
{
	if (!pParamsIn)
		OOCORE_THROW_ERRNO(EINVAL);

	// Process an incoming request...
	try
	{
		ObjectPtr<Remoting::IWireStub> ptrStub;
		uint32_t method_id;
		
		// Read the stub id and method id
		uint32_t stub_id = pParamsIn->ReadUInt32();
		if (stub_id == 0)
		{
			// It's a static interface call...
			// N.B. This will always use standard marshalling

			// Read the oid and iid
			guid_t oid;
			Omega::MetaInfo::wire_read(this,pParamsIn,oid);
			guid_t iid;
			Omega::MetaInfo::wire_read(this,pParamsIn,iid);

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

				std::map<uint32_t,ObjectPtr<Remoting::IWireStub> >::const_iterator i=m_mapStubIds.find(stub_id);
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
	catch (IException* pE)
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
			pStream->WriteByte(1);
			MetaInfo::wire_write(this,pStream,oid);
			MetaInfo::wire_write(this,pStream,iid);

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
	ObjectPtr<Remoting::IWireStub> ptrStub;
	uint32_t uId = 0;
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
		m_mapStubIds.insert(std::map<uint32_t,ObjectPtr<Remoting::IWireStub> >::value_type(uId,ptrStub));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	// Write out the data
	pStream->WriteByte(0);
	MetaInfo::wire_write(this,pStream,iid);
	pStream->WriteUInt32(uId);
}

void StdObjectManager::UnmarshalInterface(Serialize::IFormattedStream* /*pStream*/, const guid_t& /*iid*/, IObject*& /*pObject*/)
{
	void* TODO;
}

void StdObjectManager::ReleaseStub(uint32_t uId)
{
	try
	{
		ACE_GUARD_REACTION(ACE_Recursive_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

		std::map<uint32_t,ObjectPtr<Remoting::IWireStub> >::iterator i=m_mapStubIds.find(uId);
		if (i==m_mapStubIds.end())
			OOCORE_THROW_ERRNO(EINVAL);

		m_mapStubIds.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

Omega::Serialize::IFormattedStream* StdObjectManager::CreateOutputStream()
{
	return m_ptrChannel->CreateOutputStream();
}

Omega::Serialize::IFormattedStream* StdObjectManager::SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pStream)
{
	return m_ptrChannel->SendAndReceive(attribs,pStream);
}

void StdObjectManager::CreateUnboundProxy(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
/*	if (pObject)
		pObject->Release();

	ObjectPtr<ObjectImpl<UnboundProxy> > ptrProxy = ObjectImpl<UnboundProxy>::CreateObjectPtr();
	ptrProxy->init(this,oid);
	
	pObject = ptrProxy->QueryInterface(iid);*/
}
