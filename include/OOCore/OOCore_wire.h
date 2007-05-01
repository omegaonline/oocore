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
			virtual string_t ReadString() = 0;

			virtual void WriteBoolean(bool_t val) = 0;
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
			virtual void WriteString(const string_t& val) = 0;
		};
		OMEGA_DECLARE_IID(IFormattedStream);
	}

	namespace Remoting
	{
		enum MethodAttributes
		{
			asynchronous = 1,
			encrypted = 2
		};
		typedef uint16_t MethodAttributes_t;
	}

	namespace MetaInfo
	{
		interface IWireManager : public IObject
		{
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseStub(uint32_t id) = 0;
			virtual Serialize::IFormattedStream* CreateOutputStream() = 0;
			virtual Serialize::IFormattedStream* SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pParams) = 0;
		};
		OMEGA_DECLARE_IID(IWireManager);

		interface IWireStub : public IObject
		{
			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		};

		interface IWireProxy : public IObject
		{
			virtual void WriteKey(Serialize::IFormattedStream* pStream) = 0;
		};
		OMEGA_DECLARE_IID(IWireProxy);

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, byte_t& val)
		{
			val = pStream->ReadByte();
		}

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, bool_t& val)
		{
			val = pStream->ReadBoolean();
		}

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint16_t& val)
		{
			val = pStream->ReadUInt16();
		}

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint32_t& val)
		{
			val = pStream->ReadUInt32();
		}

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, uint64_t& val)
		{
			val = pStream->ReadUInt64();
		}

		inline static void wire_read(IWireManager*, Serialize::IFormattedStream* pStream, guid_t& val)
		{
			val.Data1 = pStream->ReadUInt32();
			val.Data2 = pStream->ReadUInt16();
			val.Data3 = pStream->ReadUInt16();
			uint32_t bytes = 8;
			pStream->ReadBytes(bytes,val.Data4);
			if (bytes != 8)
				OMEGA_THROW("Bad read!");
		}

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

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const uint64_t& val)
		{
			pStream->WriteUInt64(val);
		}

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const guid_t& val)
		{
			pStream->WriteUInt32(val.Data1);
			pStream->WriteUInt16(val.Data2);
			pStream->WriteUInt16(val.Data3);
			pStream->WriteBytes(8,val.Data4);
		}

		inline static void wire_write(IWireManager*, Serialize::IFormattedStream* pStream, const string_t& val)
		{
			pStream->WriteString(val);
		}

		template <class I>
		inline static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, I* val, const guid_t iid = iid_traits<I>::GetIID())
		{
			pManager->MarshalInterface(pStream,iid,val);
		}

		template <class T>
		struct std_wire_type
		{
			typedef T type;

			static void init(type& val)
			{
				val = default_value<type>::value;
			}

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val)
			{
				wire_read(pManager,pStream,val);
			}

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const type& val)
			{
				wire_write(pManager,pStream,val);
			}

			static void no_op(bool)
			{}
		};

		template <class T>
		struct std_wire_type<const T>
		{
			typedef typename interface_info<T>::wire_type::type type;

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val)
			{
				interface_info<T>::wire_type::read(pManager,pStream,val);
			}

			/*static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val, uint32_t cbSize)
			{
				interface_info<T>::wire_type::read(pManager,pStream,val,cbSize);
			}*/

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const type& val)
			{
				interface_info<T>::wire_type::write(pManager,pStream,val);
			}

			static void no_op(bool)
			{}
		};

		template <class T>
		struct std_wire_type<T&>
		{
			typedef typename interface_info<T>::wire_type::type type;

			static void init(type& val)
			{
				interface_info<T>::wire_type::init(val);
			}

			static void init(type& val, const guid_t& iid)
			{
				interface_info<T>::wire_type::init(val,iid);
			}

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val)
			{
				interface_info<T>::wire_type::read(pManager,pStream,val);
			}

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val, const guid_t& iid)
			{
				interface_info<T>::wire_type::read(pManager,pStream,val,iid);
			}

			/*static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& val, uint32_t cbSize)
			{
				interface_info<T>::wire_type::read(pManager,pStream,val,cbSize);
			}*/

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const type& val)
			{
				interface_info<T>::wire_type::write(pManager,pStream,val);
			}

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const type& val, const guid_t& iid)
			{
				interface_info<T>::wire_type::write(pManager,pStream,val,iid);
			}

			static void no_op(bool, uint32_t)
			{}

			static void no_op(bool, const guid_t& = guid_t::NIL, uint32_t = 0)
			{}
		};

		template <class T>
		struct std_wire_type_array
		{
			template <class A>
			struct auto_ptr
			{
				auto_ptr() : m_pVals(0)
				{}

				~auto_ptr()
				{
					delete [] m_pVals;
				}

				void init(uint32_t cbSize)
				{
					OMEGA_NEW(m_pVals,A[cbSize]);
					if (!m_pVals)
						OMEGA_THROW("Out of memory!");
				}

				operator A*()
				{
					return m_pVals;
				}

			private:
				A* m_pVals;
			};
			typedef auto_ptr<typename interface_info<T>::wire_type::type> type;

			static void init(type& val, uint32_t cbSize)
			{
				val.init(cbSize);
			}

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, typename interface_info<T>::wire_type::type* pVals, uint32_t cbSize)
			{
				for (uint32_t i=0;i<cbSize;++i)
					interface_info<T>::wire_type::read(pManager,pStream,pVals[i]);
			}

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const typename interface_info<T>::wire_type::type* pVals, uint32_t cbSize)
			{
				for (uint32_t i=0;i<cbSize;++i)
					interface_info<T>::wire_type::write(pManager,pStream,pVals[i]);
			}

			static void no_op(bool, uint32_t)
			{}
		};

		template <>
		struct std_wire_type_array<void>
		{
			typedef void* type;

			static void read(IWireManager*, Serialize::IFormattedStream*, type&)
			{}

			static void write(IWireManager*, Serialize::IFormattedStream*, const type&)
			{}
		};

		template <class I>
		struct iface_wire_type
		{
			typedef I* type;

			static void init(type& val, const guid_t& = guid_t::NIL)
			{
				val = 0;
			}

			static void read(IWireManager* pManager, Serialize::IFormattedStream* pStream, type& pI, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_read(pManager,pStream,pI,iid);
			}

			static void write(IWireManager* pManager, Serialize::IFormattedStream* pStream, const type& pI, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_write(pManager,pStream,pI,iid);
			}

			static void no_op(bool, const guid_t& = guid_t::NIL)
			{}
		};

		template <class I_WireProxy, class I>
		class WireProxyImpl : public IObject
		{
			struct Contained : public I_WireProxy
			{
				Contained(IObject* pOuter, IWireManager* pManager) :
					I_WireProxy(pManager), m_pOuter(pOuter)
				{ }

				virtual void AddRef()
				{
					m_pOuter->AddRef();
				}
				virtual void Release()
				{
					m_pOuter->Release();
				}
				virtual IObject* QueryInterface(const guid_t& iid)
				{
					return m_pOuter->QueryInterface(iid);
				}

			private:
				IObject* m_pOuter;

				Contained(const Contained&) {};
				Contained& operator = (const Contained&) {};
			};
			Contained					m_contained;
			AtomicOp<uint32_t>::type	m_refcount;

			WireProxyImpl(const WireProxyImpl&) {};
			WireProxyImpl& operator = (const WireProxyImpl&) {};

		public:
			WireProxyImpl(IObject* pOuter, IWireManager* pManager) : m_contained(pOuter,pManager), m_refcount(1)
			{}

			virtual ~WireProxyImpl()
			{}

			static IObject* Create(IObject* pOuter, IWireManager* pManager)
			{
				WireProxyImpl* pRet = 0;
				OMEGA_NEW(pRet,WireProxyImpl(pOuter,pManager));
				return pRet;
			}

		// IObject members
		public:
			void AddRef()
			{
				++m_refcount;
			}

			void Release()
			{
				if (--m_refcount==0)
					delete this;
			}

			IObject* QueryInterface(const guid_t& iid)
			{
				if (iid==IID_IObject)
				{
					++m_refcount;
					return this;
				}
				else
					return m_contained.Internal_QueryInterface(iid);
			}
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
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OMEGA_SOURCE_INFO);
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

			IObject* QueryInterface(const guid_t&)
			{
                return 0;
			}

			typedef void (*MethodTableEntry)(void* pParam, I* pI, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut);

			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t /*timeout*/)
			{
				static const MethodTableEntry MethodTable[] =
				{
					AddRef_Wire,
					Release_Wire,
					QueryInterface_Wire
				};

				if (method_id < MethodCount)
					MethodTable[method_id](this,m_pI,pParamsIn,pParamsOut);
				else
					OMEGA_THROW("Invalid method index");
			}
			static const uint32_t MethodCount = 3;

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
				interface_info<const guid_t&>::wire_type::type iid;
				interface_info<const guid_t&>::wire_type::read(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsIn,iid);
				interface_info<IObject*>::wire_type::type retval(pI->QueryInterface(iid));
				interface_info<IObject*>::wire_type::write(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,retval,iid);
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

		template <class I>
		struct IObject_WireProxy : public I
		{
			IObject_WireProxy(IWireManager* pManager) : m_pManager(pManager)
			{
				m_pManager->AddRef();
			}

			virtual ~IObject_WireProxy()
			{
				m_pManager->Release();
			}

			virtual IObject* Internal_QueryInterface(const guid_t&)
			{
				return 0;
			}

			void WriteKey(Serialize::IFormattedStream* pParams)
			{
				if (!m_ptrProxy)
				{
					m_ptrProxy = static_cast<IWireProxy*>(this->QueryInterface(IID_IWireProxy));
					if (!m_ptrProxy)
						INoInterfaceException::Throw(IID_IWireProxy,OMEGA_SOURCE_INFO);

					m_ptrProxy->Release();
				}

				m_ptrProxy->WriteKey(pParams);
			}

			static const uint32_t MethodCount = 3;

		protected:
			IWireManager*              m_pManager;
			auto_iface_ptr<IWireProxy> m_ptrProxy;
		};

		template <class I, class Base>
		struct IException_WireStub : public Base
		{
			IException_WireStub(IWireManager* pManager, IObject* pObj, uint32_t id) : Base(pManager,pObj,id)
			{}

			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout)
			{
				static const typename Base::MethodTableEntry MethodTable[] =
				{
					ActualIID_Wire,
					Cause_Wire,
					Description_Wire,
					Source_Wire
				};

				if (method_id < Base::MethodCount)
					Base::Invoke(method_id,pParamsIn,pParamsOut,timeout);
				else if (method_id < MethodCount)
					MethodTable[method_id - Base::MethodCount](this,Base::m_pI,pParamsIn,pParamsOut);
				else
					OMEGA_THROW("Invalid method index");
			}
			static const uint32_t MethodCount = Base::MethodCount + 4;

			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(0,guid_t,ActualIID,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(0,IException*,Cause,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(0,string_t,Description,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(0,string_t,Source,0,())

		private:
			IException_WireStub() {};
			IException_WireStub(const IException_WireStub&) {};
			IException_WireStub& operator =(const IException_WireStub&) {};
		};

		template <class I, class Base>
		struct IException_WireProxy : public Base
		{
			IException_WireProxy(IWireManager* pManager) : Base(pManager)
			{ }

			virtual IObject* Internal_QueryInterface(const guid_t& iid)
			{
				if (iid == IID_IException)
				{
					this->AddRef();
					return this;
				}

				return Base::Internal_QueryInterface(iid);
			}

			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(0,guid_t,ActualIID,0,()) 0;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(0,IException*,Cause,0,()) 1;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(0,string_t,Description,0,()) 2;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(0,string_t,Source,0,()) 3;

			static const uint32_t MethodCount = Base::MethodCount + 4;

		private:
			IException_WireProxy() {};
			IException_WireProxy(const IException_WireProxy&) {};
			IException_WireProxy& operator =(const IException_WireProxy&) {};
		};

		OMEGA_QI_MAGIC(Omega,IObject)
		OMEGA_QI_MAGIC(Omega,IException)
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
	OMEGA_METHOD(string_t,ReadString,0,())

	OMEGA_METHOD_VOID(WriteBoolean,1,((in),bool_t,val))
	OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
	OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
	OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
	OMEGA_METHOD_VOID(WriteString,1,((in),const string_t&,val))
)

OMEGA_EXPORT_INTERFACE
(
	Omega::MetaInfo, IWireManager,
	0xd327ea93, 0x660c, 0x408d, 0x81, 0x57, 0x36, 0xba, 0x3b, 0xee, 0xc3, 0x2b,

	// Methods
	OMEGA_METHOD_VOID(MarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD_VOID(ReleaseStub,1,((in),Omega::uint32_t,id))
	OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,0,())
	OMEGA_METHOD(Serialize::IFormattedStream*,SendAndReceive,2,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pParams))
)

OMEGA_DEFINE_IID(Omega::MetaInfo,IWireProxy,0xe52b96ce, 0xe4ac, 0x4327, 0x91, 0x17, 0x48, 0x68, 0x4c, 0x8a, 0x38, 0x0);

#endif // OOCORE_WIRE_H_INCLUDED_

