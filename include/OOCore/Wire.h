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

	namespace System
	{
		namespace MetaInfo
		{
			interface IWireStub : public IObject
			{
				virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut) = 0;
				virtual IObject* GetStubObject() = 0;
			};

			interface IWireProxy : public IObject
			{
				virtual void WriteKey(Serialize::IFormattedStream* pStream) = 0;
			};

			interface IWireManager : public IObject
			{
				virtual void MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
				virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
				virtual void ReleaseStub(uint32_t id) = 0;
				virtual void CreateOutputStream(IObject* pOuter, Serialize::IFormattedStream*& pStream) = 0;
				virtual Serialize::IFormattedStream* SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pParams, uint16_t timeout = 15000) = 0;
			};
		}
	}
}

OMEGA_DEFINE_IID(Omega::Serialize, IStream, "{D1072F9B-3E7C-4724-9246-46DC111AE69F}")
OMEGA_DEFINE_IID(Omega::Serialize, IFormattedStream, "{044E0896-8A60-49e8-9143-5B1F01D4AE4C}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireProxy, "{000B5251-8457-4ed5-93A2-DA1658CE1DCC}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireManager, "{1C288214-61CD-4bb9-B44D-21813DCB0017}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireStub, "{0785F8A6-A6BE-4714-A306-D9886128A40E}")

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			OMEGA_DECLARE_FORWARDS(IStream,Omega::Serialize,IStream,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IFormattedStream,Omega::Serialize,IFormattedStream,Omega::Serialize,IStream)
			OMEGA_DECLARE_FORWARDS(IWireStub,Omega::System::MetaInfo,IWireStub,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IWireProxy,Omega::System::MetaInfo,IWireProxy,Omega,IObject)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Serialize, IStream,

				OMEGA_METHOD(byte_t,ReadByte,0,())
				OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
				OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
				OMEGA_METHOD_VOID(WriteBytes,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Serialize, IFormattedStream,

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
			typedef IFormattedStream_Impl_Safe<interface_info<Serialize::IStream>::safe_class> IFormattedStream_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireStub,

				OMEGA_METHOD_VOID(Invoke,3,((in),uint32_t,method_id,(in),Serialize::IFormattedStream*,pParamsIn,(in),Serialize::IFormattedStream*,pParamsOut))
				OMEGA_METHOD(IObject*,GetStubObject,0,())
			)
			typedef IWireStub_Impl_Safe<IObject_Safe> IWireStub_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireProxy,

				OMEGA_METHOD_VOID(WriteKey,1,((in),Serialize::IFormattedStream*,pStream))
			)
			typedef IWireProxy_Impl_Safe<IObject_Safe> IWireProxy_Safe;

			static void wire_read(Serialize::IFormattedStream* pStream, guid_t& val)
			{
				val.Data1 = pStream->ReadUInt32();
				val.Data2 = pStream->ReadUInt16();
				val.Data3 = pStream->ReadUInt16();
				uint32_t bytes = 8;
				pStream->ReadBytes(bytes,val.Data4);
				if (bytes != 8)
					OMEGA_THROW(L"Bad read!");
			}

			static void wire_write(Serialize::IFormattedStream* pStream, const guid_t& val)
			{
				pStream->WriteUInt32(val.Data1);
				pStream->WriteUInt16(val.Data2);
				pStream->WriteUInt16(val.Data3);
				pStream->WriteBytes(8,val.Data4);
			}

			template <class I>
			static void wire_read(IWireManager* pManager, Serialize::IFormattedStream* pStream, I*& val, const guid_t& iid = guid_t::Null())
			{
				IObject* pObject = 0;
				pManager->UnmarshalInterface(pStream,iid,pObject);
				val = static_cast<I*>(pObject);
			}

			template <class I>
			static void wire_write(IWireManager* pManager, Serialize::IFormattedStream* pStream, I* val, const guid_t& iid = OMEGA_UUIDOF(I))
			{
				pManager->MarshalInterface(pStream,iid,val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, byte_t& val)
			{
				return pStream->ReadByte_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, bool_t& val)
			{
				return pStream->ReadBoolean_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint16_t& val)
			{
				return pStream->ReadUInt16_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint32_t& val)
			{
				return pStream->ReadUInt32_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint64_t& val)
			{
				return pStream->ReadUInt64_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, guid_t& val)
			{
				IException_Safe* pSE = pStream->ReadUInt32_Safe(&val.Data1);
				if (pSE)
					return pSE;
				pSE = pStream->ReadUInt16_Safe(&val.Data2);
				if (pSE)
					return pSE;
				pSE = pStream->ReadUInt16_Safe(&val.Data3);
				if (pSE)
					return pSE;
				uint32_t bytes = 8;
				return pStream->ReadBytes_Safe(&bytes,val.Data4);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, string_t& val)
			{
				return pStream->ReadString_Safe(&val);
			}

			template <class I>
			static IException_Safe* wire_read(IWireManager* pManager, IFormattedStream_Safe* pStream, typename interface_info<I*>::safe_class& val, const guid_t* piid = 0)
			{
				IObject* pObject = 0;
				pManager->UnmarshalInterface(interface_info<Serialize::IFormattedStream*>::stub_functor(pStream),piid ? *piid : guid_t::Null(),pObject);
				val = typename interface_info<I*>::proxy_functor(static_cast<I*>(pObject));
				return 0;
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, byte_t val)
			{
				return pStream->WriteByte_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, bool_t val)
			{
				return pStream->WriteBoolean_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, uint16_t val)
			{
				return pStream->WriteUInt16_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, uint32_t val)
			{
				return pStream->WriteUInt32_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const uint64_t& val)
			{
				return pStream->WriteUInt64_Safe(&val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const guid_t& val)
			{
				IException_Safe* pSE = pStream->WriteUInt32_Safe(val.Data1);
				if (!pSE)
				{
					pSE = pStream->WriteUInt16_Safe(val.Data2);
					if (!pSE)
					{
						pSE = pStream->WriteUInt16_Safe(val.Data3);
						if (!pSE)
							pSE = pStream->WriteBytes_Safe(8,val.Data4);
					}
				}
				return pSE;
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const string_t& val)
			{
				return pStream->WriteString_Safe(&val);
			}

			template <class I>
			static IException_Safe* wire_write(IWireManager* pManager, IFormattedStream_Safe* pStream, typename interface_info<I*>::safe_class val, const guid_t* piid = 0)
			{
				try
				{
					pManager->MarshalInterface(typename interface_info<Serialize::IFormattedStream*>::stub_functor(pStream),piid ? *piid : OMEGA_UUIDOF(I),typename interface_info<I*>::stub_functor(val));
					return 0;
				}
				catch (IException* pE)
				{
					return return_safe_exception(pE);
				}
			}

			template <class T>
			struct std_wire_type
			{
				typedef typename interface_info<T>::safe_class type;

				static IException_Safe* init(type& val)
				{
					val = default_value<type>::value;
					return 0;
				}

				static IException_Safe* read(IWireManager*, IFormattedStream_Safe* pStream, type& val)
				{
					return wire_read(pStream,val);
				}

				static IException_Safe* write(IWireManager*, IFormattedStream_Safe* pStream, const type& val)
				{
					return wire_write(pStream,val);
				}

				static IException_Safe* no_op(bool)
				{
					return 0;
				}
			};

			template <class T>
			struct std_wire_type<const T>
			{
				typedef typename interface_info<const T>::safe_class type;

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val)
				{
					return interface_info<T>::wire_type::read(pManager,pStream,const_cast<typename interface_info<T>::safe_class&>(val));
				}

				/*static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val, uint32_t cbSize)
				{
					return interface_info<T>::wire_type::read(pManager,pStream,val,cbSize);
				}*/

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const type& val)
				{
					return interface_info<T>::wire_type::write(pManager,pStream,val);
				}

				static IException_Safe* no_op(bool)
				{
					return 0;
				}
			};

			template <class T>
			struct std_wire_type<T&>
			{
				typedef typename interface_info<T&>::safe_class type;

				static IException_Safe* init(type& val)
				{
					return interface_info<T>::wire_type::init(*val);
				}

				static IException_Safe* init(type& val, const guid_t* piid, IObject* = 0)
				{
					return interface_info<T>::wire_type::init(*val,piid);
				}

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val)
				{
					return interface_info<T>::wire_type::read(pManager,pStream,*val);
				}

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val, const guid_t* piid)
				{
					return interface_info<T>::wire_type::read(pManager,pStream,*val,piid);
				}

				/*static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val, uint32_t cbSize)
				{
					return interface_info<T>::wire_type::read(pManager,pStream,val,cbSize);
				}*/

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const type& val)
				{
					return interface_info<T>::wire_type::write(pManager,pStream,*val);
				}

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const type& val, const guid_t* piid)
				{
					return interface_info<T>::wire_type::write(pManager,pStream,*val,piid);
				}

				static IException_Safe* no_op(bool, const guid_t* = 0)
				{
					return 0;
				}

				static IException_Safe* no_op(bool, uint32_t*)
				{
					return 0;
				}

				/*static void no_op(bool, const guid_t*, uint32_t)
				{}*/
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

					IException_Safe* init(uint32_t cbSize)
					{
						try
						{
							m_alloc_size = cbSize;
							OMEGA_NEW(m_pVals,A[m_alloc_size]);
						}
						catch (IException* pE)
						{
							return return_safe_exception(pE);
						}
						return 0;
					}

					operator A*()
					{
						return m_pVals;
					}

					A& operator [](size_t n)
					{
						return m_pVals[n];
					}

					const A& operator [](size_t n) const
					{
						return m_pVals[n];
					}

					uint32_t m_alloc_size;

				private:
					A* m_pVals;
				};
				typedef auto_ptr<typename interface_info<T>::wire_type::type> type;

				static IException_Safe* init(type& val, uint32_t* cbSize)
				{
					return val.init(*cbSize);
				}

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& val, uint32_t cbSize)
				{
					IException_Safe* pSE = val.init(cbSize);
					if (pSE)
						return pSE;

					for (uint32_t i=0;i<cbSize;++i)
					{
						pSE = interface_info<T>::wire_type::read(pManager,pStream,val[i]);
						if (pSE)
							return pSE;
					}
					return 0;
				}

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const type& val, uint32_t* cbSize)
				{
					if (*cbSize > val.m_alloc_size)
						*cbSize = val.m_alloc_size;

					for (uint32_t i=0;i<*cbSize;++i)
					{
						IException_Safe* pSE = interface_info<T>::wire_type::write(pManager,pStream,val[i]);
						if (pSE)
							return pSE;
					}
					return 0;
				}

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, typename interface_info<T>::wire_type::type* pVals, uint32_t* cbSize)
				{
					if (pVals)
					{
						for (uint32_t i=0;i<*cbSize;++i)
						{
							IException_Safe* pSE = interface_info<T>::wire_type::read(pManager,pStream,pVals[i]);
							if (pSE)
								return pSE;
						}
					}
					return 0;
				}

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const typename interface_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					if (pVals)
					{
						for (uint32_t i=0;i<cbSize;++i)
						{
							IException_Safe* pSE = interface_info<T>::wire_type::write(pManager,pStream,pVals[i]);
							if (pSE)
								return pSE;
						}
					}
					return 0;
				}

				static IException_Safe* no_op(bool, uint32_t)
				{
					return 0;
				}

				static IException_Safe* no_op(bool, uint32_t*)
				{
					return 0;
				}
			};

			template <>
			struct std_wire_type_array<void>
			{
				typedef void* type;

				static IException_Safe* read(IWireManager*, IFormattedStream_Safe*, type&)
				{
					return 0;
				}

				static IException_Safe* write(IWireManager*, IFormattedStream_Safe*, const type&)
				{
					return 0;
				}
			};

			template <class I>
			struct iface_wire_type
			{
				typedef typename interface_info<I*>::safe_class type;

				static IException_Safe* init(type& val, const guid_t* = 0)
				{
					val = 0;
					return 0;
				}

				static IException_Safe* read(IWireManager* pManager, IFormattedStream_Safe* pStream, type& pI, const guid_t* piid = 0)
				{
					return wire_read<I>(pManager,pStream,pI,piid);
				}

				static IException_Safe* write(IWireManager* pManager, IFormattedStream_Safe* pStream, const type& pI, const guid_t* piid = 0)
				{
					return wire_write<I>(pManager,pStream,pI,piid);
				}

				static IException_Safe* no_op(bool, const guid_t* = 0)
				{
					return 0;
				}
			};

			inline void RegisterWireFactories(const guid_t& iid, void* pfnProxy, void* pfnStub);

			template <class S>
			IWireStub_Safe* CreateWireStub(IWireManager* pManager, IObject_Safe* pObject, uint32_t id)
			{
				S* pS;
				OMEGA_NEW(pS,S(pManager,pObject,id));
				return pS;
			}

			template <class I_WireProxy>
			class WireProxyImpl : public IObject_Safe
			{
				struct Contained : public I_WireProxy
				{
					Contained(IObject_Safe* pOuter, IWireManager* pManager, uint32_t id) :
						I_WireProxy(pManager,id), m_pOuter(pOuter)
					{
					}

					virtual void OMEGA_CALL AddRef_Safe()
					{
						m_pOuter->AddRef_Safe();
					}
					virtual void OMEGA_CALL Release_Safe()
					{
						m_pOuter->Release_Safe();
					}
					virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS)
					{
						return m_pOuter->QueryInterface_Safe(piid,ppS);
					}

				private:
					IObject_Safe* m_pOuter;

					Contained(const Contained&) {};
					Contained& operator = (const Contained&) {};
				};
				Contained                  m_contained;
				System::AtomicOp<uint32_t> m_refcount;

				WireProxyImpl(const WireProxyImpl&) {};
				WireProxyImpl& operator = (const WireProxyImpl&) {};

			public:
				WireProxyImpl(IObject_Safe* pOuter, IWireManager* pManager, uint32_t id) : m_contained(pOuter,pManager,id), m_refcount(1)
				{}

				virtual ~WireProxyImpl()
				{}

				static void Create(IObject_Safe* pOuter, IWireManager* pManager, uint32_t id, IObject_Safe** ppObject)
				{
					WireProxyImpl* pRet = 0;
					OMEGA_NEW(pRet,WireProxyImpl(pOuter,pManager,id));
					*ppObject = pRet;
				}

			// IObject members
			public:
				virtual void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
						delete this;
				}

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** retval)
				{
					*retval = 0;
					if (*piid==OMEGA_UUIDOF(SafeProxy))
						return 0;
					else if (*piid==OMEGA_UUIDOF(IObject))
					{
						++m_refcount;
						*retval = this;
						return 0;
					}
					else
						return m_contained.Internal_QueryInterface_Safe(*piid,*retval);
				}
			};

			template <class I>
			struct IObject_WireStub : public IWireStub_Safe
			{
				IObject_WireStub(IWireManager* pManager, IObject_Safe* pObj, uint32_t id) :
					m_pManager(pManager), m_id(id), m_refcount(1), m_remote_refcount(1)
				{
					m_ptrI = static_cast<I*>(pObj);
				}

				virtual ~IObject_WireStub() {}

				virtual void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
						delete this;
				}

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t*, IObject_Safe** retval)
				{
					*retval = 0;
					return 0;
				}

				virtual IException_Safe* OMEGA_CALL GetStubObject_Safe(IObject_Safe** ppObject)
				{
					return m_ptrI->QueryInterface_Safe(&OMEGA_UUIDOF(IObject),ppObject);
				}

				typedef IException_Safe* (*MethodTableEntry)(void* pParam, I* pI, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut);

				virtual IException_Safe* OMEGA_CALL Invoke_Safe(uint32_t method_id, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					static const MethodTableEntry MethodTable[] =
					{
						AddRef_Wire,
						Release_Wire,
						QueryInterface_Wire
					};

					if (method_id < MethodCount)
						return MethodTable[method_id](this,m_ptrI,pParamsIn,pParamsOut);
					else
						return return_safe_exception(IException::Create(L"Invalid method index"));
				}
				static const uint32_t MethodCount = 3;

				IWireManager*          m_pManager;
				auto_iface_safe_ptr<I> m_ptrI;

			private:
				IObject_WireStub(const IObject_WireStub&) {};
				IObject_WireStub& operator =(const IObject_WireStub&) {};

				const uint32_t             m_id;
				System::AtomicOp<uint32_t> m_refcount;
				System::AtomicOp<uint32_t> m_remote_refcount;

				static IException_Safe* AddRef_Wire(void* pParam,I*,IFormattedStream_Safe*,IFormattedStream_Safe*)
				{
					++(static_cast<IObject_WireStub<I>*>(pParam)->m_refcount);
					return 0;
				}

				static IException_Safe* Release_Wire(void* pParam,I*,IFormattedStream_Safe*,IFormattedStream_Safe*)
				{
					try
					{
						IObject_WireStub<I>* pThis = static_cast<IObject_WireStub<I>*>(pParam);
						if (--pThis->m_refcount==0)
							pThis->m_pManager->ReleaseStub(pThis->m_id);
						return 0;
					}
					catch (IException* pE)
					{
						return return_safe_exception(pE);
					}
				}

				static IException_Safe* QueryInterface_Wire(void* pParam, I* pI, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					guid_t iid;
					IException_Safe* pSE = wire_read(pParamsIn,iid);
					if (pSE)
						return pSE;

					IObject_Safe* p;
					pSE = pI->QueryInterface_Safe(&iid,&p);
					if (pSE)
						return pSE;

					auto_iface_safe_ptr<IObject_Safe> ptr(p);
					return wire_write<IObject>(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,p,&iid);
				}
			};

			template <class I>
			struct IObject_WireProxy : public I
			{
				IObject_WireProxy(IWireManager* pManager, uint32_t id) :
					m_pManager(pManager), m_uId(id)
				{
					m_pManager->AddRef();
				}

				virtual ~IObject_WireProxy()
				{
					m_pManager->Release();
				}

                virtual IException_Safe* Internal_QueryInterface_Safe(const guid_t& iid, IObject_Safe*& pObject)
				{
					IException_Safe* pSE = 0;
					if (m_uId == 0)
						pObject = 0;
					else
					{
						auto_iface_safe_ptr<IFormattedStream_Safe> pParamsOut;
						pSE = CreateOutputStream(pParamsOut);
						if (pSE)
							return pSE;
						pSE = WriteKey(pParamsOut);
						if (pSE)
							return pSE;
						pSE = wire_write(pParamsOut,(uint32_t)2);
						if (pSE)
							return pSE;
						pSE = interface_info<const guid_t&>::wire_type::write(this->m_pManager,pParamsOut,&iid);
						if (pSE)
							return pSE;
						auto_iface_safe_ptr<IFormattedStream_Safe> pParamsIn;
						pSE = SendAndReceive(0,pParamsOut,pParamsIn);
						if (pSE)
							return pSE;
						pSE = interface_info<IObject*>::wire_type::read(this->m_pManager,pParamsIn,pObject,&iid);
					}
					return pSE;
				}

				IException_Safe* WriteKey(IFormattedStream_Safe* pParams)
				{
					IException_Safe* pSE = pParams->WriteUInt32_Safe(m_uId);
					if (!pSE && !m_uId)
					{
						IObject_Safe* pObj = 0;
						pSE = this->QueryInterface_Safe(&OMEGA_UUIDOF(IWireProxy),&pObj);
						if (pSE)
							return pSE;
						if (!pObj)
							return return_safe_exception(INoInterfaceException::Create(OMEGA_UUIDOF(IWireProxy),OMEGA_SOURCE_INFO));

						// We don't need the extra ref here, cos its to us
						pObj->Release_Safe();

						pSE = static_cast<IWireProxy_Safe*>(pObj)->WriteKey_Safe(pParams);
					}
					return pSE;
				}

				IException_Safe* CreateOutputStream(auto_iface_safe_ptr<IFormattedStream_Safe>& pStream)
				{
					try
					{
						Serialize::IFormattedStream* p = 0;
						m_pManager->CreateOutputStream(0,p);
						pStream = interface_info<Serialize::IFormattedStream*>::proxy_functor(p);
						return 0;
					}
					catch (IException* pE)
					{
						return return_safe_exception(pE);
					}
				}

				IException_Safe* SendAndReceive(Remoting::MethodAttributes_t attribs, IFormattedStream_Safe* pParamsOut, auto_iface_safe_ptr<IFormattedStream_Safe>& pParamsIn, Omega::uint16_t timeout = 0)
				{
					try
					{
						Serialize::IFormattedStream* p = m_pManager->SendAndReceive(attribs,interface_info<Serialize::IFormattedStream*>::stub_functor(pParamsOut),timeout);
						pParamsIn = interface_info<Serialize::IFormattedStream*>::proxy_functor(p);
						return 0;
					}
					catch (IException* pE)
					{
						return return_safe_exception(pE);
					}
				}

				static const uint32_t MethodCount = 3;

			protected:
				IWireManager*  m_pManager;

			private:
				uint32_t       m_uId;
			};

			OMEGA_WIRE_MAGIC(Omega,IObject)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega, IException,

				OMEGA_METHOD(guid_t,ActualIID,0,())
				OMEGA_METHOD(IException*,Cause,0,())
				OMEGA_METHOD(string_t,Description,0,())
				OMEGA_METHOD(string_t,Source,0,())
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega::Serialize, IStream,

				OMEGA_METHOD(byte_t,ReadByte,0,())
				OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
				OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
				OMEGA_METHOD_VOID(WriteBytes,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega::Serialize, IFormattedStream,

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
		}
	}
}

OOCORE_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const Omega::guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub));
void Omega::System::MetaInfo::RegisterWireFactories(const guid_t& iid, void* pfnProxy, void* pfnStub)
{
	Omega_RegisterWireFactories(iid,pfnProxy,pfnStub);
}

//OMEGA_DEFINE_INTERFACE_DERIVED_NOWIRE
//(
//	Omega::System::MetaInfo, IWireManager, Omega, IObject, "{1C288214-61CD-4bb9-B44D-21813DCB0017}",
//
//	// Methods
//	OMEGA_METHOD_VOID(MarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
//	OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
//	OMEGA_METHOD_VOID(ReleaseStub,1,((in),Omega::uint32_t,id))
//	OMEGA_METHOD_VOID(CreateOutputStream,2,((in),IObject*,pOuter,(out),Serialize::IFormattedStream*&,pStream))
//	OMEGA_METHOD(Serialize::IFormattedStream*,SendAndReceive,3,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pParams,(in),uint16_t,timeout))
//)

#endif // OOCORE_WIRE_H_INCLUDED_
