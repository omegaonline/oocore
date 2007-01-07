#ifndef OOCORE_WIRE_H_INCLUDED_
#define OOCORE_WIRE_H_INCLUDED_

namespace Omega
{	
	namespace Serialize
	{
		interface IStream : public IObject
		{
			virtual byte_t ReadByte() = 0;
			virtual void ReadBytes(uint32_t cbBytes, byte_t* val) = 0;
			virtual void WriteByte(byte_t val) = 0;
			virtual void WriteBytes(uint32_t cbBytes, const byte_t* val) = 0;
		};
		OMEGA_DECLARE_IID(IStream);

		interface IFormattedStream : public IStream
		{
			virtual bool_t ReadBoolean() = 0;
			virtual uint16_t ReadUInt16() = 0;
			virtual uint32_t ReadUInt32() = 0;
			virtual uint64_t ReadUInt64() = 0;
			virtual guid_t ReadGuid() = 0;
			
			virtual void WriteBoolean(bool_t val) = 0;
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
			virtual void WriteGuid(const guid_t& val) = 0;
		};
		OMEGA_DECLARE_IID(IFormattedStream);
	}

	namespace MetaInfo
	{
		interface IWireManager : public IObject
		{
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, IObject* pObject, const guid_t& iid) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject** pObject) = 0;
		};

		interface IWireStub : public IObject
		{
			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		};

		template <class T>
		static void wire_read(IWireManager* pManager, Serialize::IFormattedStream*, T&);

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, byte_t& val)
		{
			val = pStream->ReadByte();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, bool_t& val)
		{
			val = pStream->ReadBoolean();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint16_t& val)
		{
			val = pStream->ReadUInt16();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint32_t& val)
		{
			val = pStream->ReadUInt32();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint64_t& val)
		{
			val = pStream->ReadUInt64();
		}

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, guid_t& val)
		{
			val = pStream->ReadGuid();
		}

		template <class iface>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, iface** ppI, const guid_t& iid = iid_traits<iface>::GetIID())
		{
			pManager->UnmarshalInterface(pStream,iid,ppI);
		}

		template <class iface>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, iface* pI, const guid_t& iid = iid_traits<iface>::GetIID())
		{
			pManager->MarshalInterface(pStream,pI,iid);
		}

		template <class T>
		struct std_wire_functor
		{
			std_wire_functor(IWireManager* pManager) :
				m_pManager(pManager)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream) :
				m_pManager(pManager)
			{
				read(pStream);
			}

			void read(Serialize::IFormattedStream* pStream)
			{
				wire_read(m_pManager,pStream,m_val);
			}

			void write(Serialize::IFormattedStream* pStream)
			{
				wire_write(m_pManager,pStream,m_val);
			}

			std_wire_functor& operator = (const guid_t& val)
			{
				m_val = val;
				return *this;
			}

			operator T()
			{
				return m_val;
			}

			IWireManager*	m_pManager;
			T				m_val;
		};

		template <class T>
		struct std_wire_functor<T*>
		{
			std_wire_functor(IWireManager* pManager, std_wire_functor<uint32_t>& cbSize) : 
				m_pManager(pManager), m_pVals(0), m_cbSize(cbSize)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, std_wire_functor<uint32_t>& cbSize) :
				m_pManager(pManager), m_pVals(0), m_cbSize(cbSize)
			{
				read(pStream);
			}

			~std_wire_functor()
			{
				if (m_pVals)
					delete [] m_pVals;
			}

			void read(Serialize::IFormattedStream* pStream)
			{
				if (m_pVals)
				{
					delete [] m_pVals;
					m_pVals = 0;
				}

				uint32_t size = static_cast<uint32_t>(cbSize);
				if (size > 0)
				{
					OMEGA_NEW(m_pVals,T[size]);

					for (uint32_t i=0;i<size;++i)
						wire_read(pManager,pStream,m_pVals[i]);
				}
			}

			void write(Serialize::IFormattedStream* pStream)
			{
				pStream->WriteGuid(m_val);
			}

			operator T*()
			{
				return m_pVals;
			}

			IWireManager*				m_pManager;
			std_wire_functor<uint32_t>&	m_cbSize;
			T*							m_pVals;
		};

		template <class I>
		struct iface_wire_functor<I*>
		{
			iface_wire_functor(IWireManager* pManager, std_wire_functor<const guid_t&>* piid = 0) :
				m_pManager(pManager), m_val(0)
			{}

			iface_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, std_wire_functor<const guid_t&>* piid = 0) :
				m_pManager(pManager), m_val(0)
			{
				read(pStream);
			}

			~iface_wire_functor()
			{
				if (m_val)
					m_val->Release();
			}

			void read(Serialize::IFormattedStream* pStream)
			{
				wire_read(m_pManager,pStream,m_val);
			}

			void write(Serialize::IFormattedStream* pStream)
			{
				wire_write(m_pManager,pStream,m_val);
			}

			iface_wire_functor& operator = (const guid_t& val)
			{
				m_val = val;
				return *this;
			}

			IWireManager*	m_pManager;
			I*				m_val;
		};

		template <class I>
		struct iface_wire_functor<I**>
		{
			iface_wire_functor(IWireManager* pManager, std_wire_functor<const guid_t&>* piid = 0) :
				m_pManager(pManager), m_val(0)
			{}

			iface_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, std_wire_functor<const guid_t&>* piid = 0) :
				m_pManager(pManager), m_val(0)
			{
				read(pStream);
			}

			~iface_wire_functor()
			{
				if (m_val)
					m_val->Release();
			}

			void read(Serialize::IFormattedStream* pStream)
			{
				wire_read(m_pManager,pStream,m_val);
			}

			void write(Serialize::IFormattedStream* pStream)
			{
				wire_write(m_pManager,pStream,m_val);
			}

			iface_wire_functor& operator = (const guid_t& val)
			{
				m_val = val;
				return *this;
			}

			IWireManager*	m_pManager;
			I*				m_val;
		};

		template <class I>
		static IWireStub* CreateWireStub(IWireManager* pManager, IObject* pObject, uint32_t id)
		{
			I* pI;
			OMEGA_NEW(pI,I(pManager,pObject,id));
			return pI;
		}

		template <class I>
		struct IObject_WireStub : public IWireStub
		{
			IObject_WireStub(IWireManager* pManager, IObject* pObj, uint32_t id) : m_id(id), m_refcount(1)
			{ 
				m_pManager = pManager;
				m_pManager->AddRef();

				m_pI = static_cast<I*>(pObj->QueryInterface(iid_traits<I>::GetIID()));
				if (!m_pI)
				{
					m_pManager->Release();
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OOCORE_FUNCNAME);
				}				
			} 

			virtual ~IObject_WireStub()
			{
				if (m_pI)
					m_pI->Release();
				
				if (m_pManager)
					m_pManager->Release();
			}

			void AddRef()
			{
				++m_refcount;
			}

			void Release()
			{
				if (--m_refcount==0)
					delete this;
			}

			IObject* QueryInterface(const guid_t& /*iid*/)
			{
				return 0;
			}

			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t /*timeout*/)
			{
				if (!this->invoke(method_id,pParamsIn,pParamsOut))
					OMEGA_THROW("Invalid method index");
			}

			typedef void (*MethodTableEntry)(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut);

			virtual bool invoke(uint32_t& method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{
				static const MethodTableEntry MethodTable[] =
				{
					AddRef_Wire,
					Release_Wire,
					QueryInterface_Wire
				};

				if (method_id >= sizeof(MethodTable)/sizeof(MethodTableEntry))
				{
					method_id -= sizeof(MethodTable)/sizeof(MethodTableEntry);
					return false;
				}					

				MethodTable[method_id](this,m_pI,pParamsIn,pParamsOut);
				return true;
			}

			inline static void AddRef_Wire(void* pParam, I*,Serialize::IFormattedStream*,Serialize::IFormattedStream*)
			{ 
				static_cast<IObject_WireStub<I>*>(pParam)->AddRef();
			}

			inline static void Release_Wire(void* pParam, I*,Serialize::IFormattedStream*,Serialize::IFormattedStream*)
			{ 
				static_cast<IObject_WireStub<I>*>(pParam)->Release();
			}

			inline static void QueryInterface_Wire(void* /*pParam*/, I* /*pI*/, Serialize::IFormattedStream* /*pParamsIn*/, Serialize::IFormattedStream* /*pParamsOut*/)
			{ 
				//void* TODO;
			}

			const uint32_t m_id;
			I* m_pI;
			IWireManager* m_pManager;

		private:
			IObject_WireStub() {};
			IObject_WireStub(const IObject_WireStub&) {};
			IObject_WireStub& operator =(const IObject_WireStub&) {};

			AtomicOp<uint32_t>::type m_refcount;
		};

		template <class I, class Base>
		struct IException_WireStub : public Base
		{
			IException_WireStub(IWireManager* pManager, IObject* pObj, uint32_t id) : Base(pManager,pObj,id)
			{}

			virtual bool invoke(uint32_t& method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{
				static const MethodTableEntry MethodTable[] =
				{
					GetActualIID_Wire,
					Cause_Wire,
					Description_Wire,
					Source_Wire
				};

				// Try the base class first
				if (Base::invoke(method_id,pParamsIn,pParamsOut))
					return true;

				if (method_id >= sizeof(MethodTable)/sizeof(MethodTableEntry))
				{
					method_id -= sizeof(MethodTable)/sizeof(MethodTableEntry);
					return false;
				}					

				MethodTable[method_id](this,m_pI,pParamsIn,pParamsOut);
				return true;
			}

			inline static void GetActualIID_Wire(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut)
			{
				__wire__pParamsIn; __wire__pParamsOut;
				std_wire_functor<guid_t> retval(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager);
				retval = __wire__pI->GetActualIID();
				retval.write(__wire__pParamsOut);
			}

			inline static void Cause_Wire(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{ pParam; pI; pParamsIn; pParamsOut; }

			inline static void Description_Wire(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{ pParam; pI; pParamsIn; pParamsOut; }

			inline static void Source_Wire(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{ pParam; pI; pParamsIn; pParamsOut; }

		private:
			IException_WireStub() {};
			IException_WireStub(const IException_WireStub&) {};
			IException_WireStub& operator =(const IException_WireStub&) {};
		};

		

	}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Serialize, IStream,
	0x5344a2f5, 0x5d58, 0x46c4, 0xa0, 0x96, 0xe1, 0x71, 0x98, 0xa8, 0x8, 0xd2,

	// Methods
	OMEGA_METHOD(byte_t,ReadByte,0,())
	OMEGA_METHOD_VOID(ReadBytes,2,((in),uint32_t,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
	OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
	OMEGA_METHOD_VOID(WriteBytes,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
)

OMEGA_EXPORT_INTERFACE_DERIVED
(
	Omega::Serialize, IFormattedStream, Omega::Serialize, IStream,
	0xc3df15dc, 0x10c, 0x4fa0, 0x8d, 0xe6, 0x24, 0x3b, 0x13, 0xe0, 0x1c, 0xf7,
	
	// Methods
	OMEGA_METHOD(bool_t,ReadBoolean,0,())
	OMEGA_METHOD(uint16_t,ReadUInt16,0,())
	OMEGA_METHOD(uint32_t,ReadUInt32,0,())
	OMEGA_METHOD(uint64_t,ReadUInt64,0,())
	OMEGA_METHOD(guid_t,ReadGuid,0,())
	
	OMEGA_METHOD_VOID(WriteBoolean,1,((in),bool_t,val))
	OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
	OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
	OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
	OMEGA_METHOD_VOID(WriteGuid,1,((in),const guid_t&,val))
)

#endif // OOCORE_WIRE_H_INCLUDED_
