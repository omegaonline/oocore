#ifndef OOCORE_WIRE_H_INCLUDED_
#define OOCORE_WIRE_H_INCLUDED_

namespace Omega
{	
	namespace Serialize
	{
		interface IStream : public IObject
		{
			virtual byte_t ReadByte() = 0;
			virtual void ReadBytes(uint32_t& cbBytes, byte_t* val) = 0;
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
			virtual string_t ReadString() = 0;
			
			virtual void WriteBoolean(bool_t val) = 0;
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
			virtual void WriteGuid(const guid_t& val) = 0;
			virtual void WriteString(const string_t& val) = 0;
		};
		OMEGA_DECLARE_IID(IFormattedStream);
	}

	namespace MetaInfo
	{
		interface IWireManager : public IObject
		{
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, IObject* pObject, const guid_t& iid) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseStub(uint32_t id) = 0;
		};

		interface IWireStub : public IObject
		{
			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		};

		template <class T>
		static void wire_read(IWireManager*, Serialize::IFormattedStream*, T&);

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

		template <>
		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, string_t& val)
		{
			val = pStream->ReadString();
		}

		template <class I>
		inline static void wire_read(IWireManager* pManager, Serialize::IFormattedStream* pStream, I*& val, const guid_t iid = iid_traits<I>::GetIID())
		{
			IObject* pObject = 0;
			pManager->UnmarshalInterface(pStream,iid,pObject);
			val = static_cast<I*>(pObject);
		}

		template <class T>
		static void wire_write(IWireManager*, Serialize::IFormattedStream*, const T&);

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, byte_t val)
		{
			pStream->WriteByte(val);
		}

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, bool_t val)
		{
			pStream->WriteBoolean(val);
		}

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, uint16_t val)
		{
			pStream->WriteUInt16(val);
		}

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, uint32_t val)
		{
			pStream->WriteUInt32(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const uint64_t& val)
		{
			pStream->WriteUInt64(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const guid_t& val)
		{
			pStream->WriteGuid(val);
		}

		template <>
		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const string_t& val)
		{
			pStream->WriteString(val);
		}

		template <class I>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, I* val, const guid_t iid = iid_traits<I>::GetIID())
		{
			pManager->MarshalInterface(pStream,val,iid);
		}

		template <class T>
		struct std_wire_functor
		{
			std_wire_functor(IWireManager* = 0) : m_val(m_fixed)
			{}

			std_wire_functor(const T& val) : m_fixed(val), m_val(m_fixed)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream) : m_val(m_fixed)
			{
				read(pManager,pStream);	
			}

			std_wire_functor& operator = (const T& val)
			{
				m_val = val;
				return *this;
			}

			void attach(T& val_ref)
			{
				m_val = val_ref;
			}

			void read(IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{				
				wire_read(pManager,pStream,m_val);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				wire_write(pManager,pStream,m_val);
			}

			operator T&()
			{
				return m_val;
			}

		private:
			T	m_fixed;
			T&	m_val;

			std_wire_functor(const std_wire_functor&) {}
			std_wire_functor& operator = (const std_wire_functor&) {}
		};

		template <class T>
		struct std_wire_functor<const T>
		{
			std_wire_functor(IWireManager* pManager = 0) : m_actual(pManager)
			{}

			std_wire_functor(IWireManager* pManager, const guid_t& iid) : m_actual(pManager,iid)
			{}

			std_wire_functor(const T& val) : m_actual(val)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream) :
				m_actual(pManager,pStream)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize) :
				m_actual(pManager,pStream,cbSize)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid) :
				m_actual(pManager,pStream,iid)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize) :
				m_actual(pManager,pStream,iid,cbSize)
			{}

			std_wire_functor& operator = (const T& val)
			{
				m_actual = val;
				return *this;
			}

			void attach(const T& val_ref)
			{
				m_actual.attach(const_cast<T&>(val_ref));
			}

			void read(IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				m_actual.read(pManager,pStream);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				m_actual.write(pManager,pStream);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize)
			{
				m_actual.write(pManager,pStream,cbSize);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid)
			{
				m_actual.write(pManager,pStream,iid);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize)
			{
				m_actual.write(pManager,pStream,iid,cbSize);
			}

			operator const T&()
			{
				return m_actual;
			}

		private:
			typename interface_info<T>::wire_functor	m_actual;

			std_wire_functor(const std_wire_functor&) {}
			std_wire_functor& operator = (const std_wire_functor&) {}
		};

		template <class T>
		struct std_wire_functor<T&>
		{
			std_wire_functor(IWireManager* pManager) : m_actual(pManager)
			{}

			std_wire_functor(IWireManager* pManager, const guid_t& iid) : m_actual(pManager,iid)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream) :
				m_actual(pManager,pStream)
			{}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid) :
				m_actual(pManager,pStream,iid)
			{}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				m_actual.write(pManager,pStream);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid)
			{
				m_actual.write(pManager,pStream,iid);
			}

			operator T& ()
			{
				return m_actual;
			}

		private:
			typename interface_info<T>::wire_functor	m_actual;

			std_wire_functor(const std_wire_functor&) {}
			std_wire_functor& operator = (const std_wire_functor&) {}
		};

		template <class T>
		struct std_wire_functor<T*>
		{
			std_wire_functor(IWireManager*, uint32_t cbSize = 1) : 
				m_pFunctors(0), m_pVals(0), m_alloc_size(cbSize)
			{
				try
				{
					init(cbSize);
				}
				catch (...)
				{
					delete [] m_pFunctors;
					delete [] m_pVals;
					throw;
				}
			}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize = 1) :
				m_pFunctors(0), m_pVals(0), m_alloc_size(cbSize)
			{
				try
				{
					init(cbSize);

					for (uint32_t i=0;i<cbSize;++i)
						m_pFunctors[i].read(pManager,pStream);
				}
				catch (...)
				{
					delete [] m_pFunctors;
					delete [] m_pVals;
					throw;
				}
			}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize = 1) :
				m_pFunctors(0), m_pVals(0), m_alloc_size(cbSize)
			{
				try
				{
					init(cbSize);

					for (uint32_t i=0;i<cbSize;++i)
						m_pFunctors[i].read(pManager,pStream,iid);
				}
				catch (...)
				{
					delete [] m_pFunctors;
					delete [] m_pVals;
					throw;
				}
			}

			std_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t* piids, uint32_t cbSize = 1) :
				m_pFunctors(0), m_pVals(0), m_alloc_size(cbSize)
			{
				try
				{
					init(cbSize);

					for (uint32_t i=0;i<cbSize;++i)
						m_pFunctors[i].read(pManager,pStream,piids[i]);
				}
				catch (...)
				{
					delete [] m_pFunctors;
					delete [] m_pVals;
					throw;
				}
			}

			~std_wire_functor()
			{
				delete [] m_pFunctors;
				delete [] m_pVals;
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream,iid);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t* piids, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream,piids[i]);
			}

			operator T*()
			{
				return m_pVals;
			}

		private:
			typename interface_info<T>::wire_functor*	m_pFunctors;
			T*									m_pVals;
			const uint32_t						m_alloc_size;

			void init(uint32_t cbSize)
			{
				if (cbSize > 0)
				{
					OMEGA_NEW(m_pFunctors,interface_info<T>::wire_functor[cbSize]);
					OMEGA_NEW(m_pVals,T[cbSize]);

					for (uint32_t i=0;i<cbSize;++i)
						m_pFunctors[i].attach(m_pVals[i]);
				}
			}

			std_wire_functor(const std_wire_functor&) {};
			std_wire_functor& operator = (const std_wire_functor&) {};
		};

		template <class I>
		struct iface_wire_functor<I*>
		{
			iface_wire_functor(IWireManager*, const guid_t& = iid_traits<I>::GetIID()) :
				m_fixed(0), m_pI(m_fixed)
			{}

			iface_wire_functor(I* pI) :
				m_fixed(pI), m_pI(m_fixed)
			{
				if (m_pI)
					m_pI->AddRef();
			}

			iface_wire_functor(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID()) :
				m_fixed(0), m_pI(m_fixed)
			{
				read(pManager,pStream,iid);
			}

			iface_wire_functor& operator = (I* val)
			{
				if (m_pI != val)
				{
					if (m_pI)
						m_pI->Release();

					m_pI = val;

					if (m_pI)
						m_pI->AddRef();
				}
				return *this;
			}

			virtual ~iface_wire_functor()
			{
				if (m_pI)
					m_pI->Release();
			}

			void attach(I*& val_ref)
			{
				m_pI = val_ref;
			}

			void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_read(pManager,pStream,m_pI,iid);
			}

			void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID())
			{
				pManager->MarshalInterface(pStream,m_pI,iid);
			}

			operator I*&()
			{
				return m_pI;
			}

		private:
			I*				m_fixed;
			I*&				m_pI;

			iface_wire_functor(const iface_wire_functor&) {}
			iface_wire_functor& operator = (const iface_wire_functor&) {}
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
			IObject_WireStub(IWireManager* pManager, IObject* pObj, uint32_t id) : 
				m_pManager(pManager), m_pI(0), m_id(id), m_refcount(1), m_remote_refcount(1)
			{ 
				m_pI = static_cast<I*>(pObj->QueryInterface(iid_traits<I>::GetIID()));
				if (!m_pI)
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OOCORE_FUNCNAME);			
			} 

			virtual ~IObject_WireStub()
			{
				if (m_pI)
					m_pI->Release();
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

			inline static void AddRef_Wire(void* pParam,I*,Serialize::IFormattedStream*,Serialize::IFormattedStream*)
			{ 
				++(static_cast<IObject_WireStub<I>*>(pParam)->m_refcount);
			}

			inline static void Release_Wire(void* pParam,I*,Serialize::IFormattedStream*,Serialize::IFormattedStream*)
			{ 
				IObject_WireStub<I>* pThis = static_cast<IObject_WireStub<I>*>(pParam);
				if (--pThis->m_refcount==0)
					pThis->m_pManager->ReleaseStub(pThis->m_id);
			}

			inline static void QueryInterface_Wire(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
			{ 
				interface_info<const guid_t&>::wire_functor iid(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsIn);
				iface_wire_functor<IObject*> retval = pI->QueryInterface(iid);
				retval.write(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,iid);
			}

			IWireManager* m_pManager;
			I* m_pI;

		private:
			IObject_WireStub() {};
			IObject_WireStub(const IObject_WireStub&) {};
			IObject_WireStub& operator =(const IObject_WireStub&) {};

			const uint32_t m_id;
			AtomicOp<uint32_t>::type m_refcount;
			AtomicOp<uint32_t>::type m_remote_refcount;
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
				__wire__pParam; __wire__pParamsIn; __wire__pParamsOut;
				interface_info<guid_t>::wire_functor retval = __wire__pI->GetActualIID();
				retval.write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut);
			}

			inline static void Cause_Wire(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut)
			{
				__wire__pParam; __wire__pParamsIn; __wire__pParamsOut;
				interface_info<IException*>::wire_functor retval = __wire__pI->Cause();
				retval.write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut);
			}

			inline static void Description_Wire(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut)
			{
				__wire__pParam; __wire__pParamsIn; __wire__pParamsOut;
				interface_info<string_t>::wire_functor retval = __wire__pI->Description();
				retval.write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut);
			}

			inline static void Source_Wire(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut)
			{
				__wire__pParam; __wire__pParamsIn; __wire__pParamsOut;
				interface_info<string_t>::wire_functor retval = __wire__pI->Source();
				retval.write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut);
			}

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
	OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
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
	OMEGA_METHOD(string_t,ReadString,0,())
	
	OMEGA_METHOD_VOID(WriteBoolean,1,((in),bool_t,val))
	OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
	OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
	OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
	OMEGA_METHOD_VOID(WriteGuid,1,((in),const guid_t&,val))
	OMEGA_METHOD_VOID(WriteString,1,((in),const string_t&,val))
)

#endif // OOCORE_WIRE_H_INCLUDED_

