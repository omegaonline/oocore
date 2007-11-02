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
				virtual bool_t SupportsInterface(const guid_t& iid) = 0;
			};

			interface IWireProxy : public IObject
			{
				virtual void WriteKey(Serialize::IFormattedStream* pStream) = 0;
			};

			interface IWireManager : public IObject
			{
				virtual void MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
				virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
				virtual void ReleaseMarshalData(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
				virtual void ReleaseStub(uint32_t id) = 0;
				virtual Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0) = 0;
				virtual IException* SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv, uint16_t timeout = 15000) = 0;
			};
		}
	}
}

OMEGA_DEFINE_IID(Omega::Serialize, IStream, "{D1072F9B-3E7C-4724-9246-46DC111AE69F}")
OMEGA_DEFINE_IID(Omega::Serialize, IFormattedStream, "{044E0896-8A60-49e8-9143-5B1F01D4AE4C}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireStub, "{0785F8A6-A6BE-4714-A306-D9886128A40E}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireProxy, "{0D4BE871-5AD0-497b-A018-EDEA8C17255B}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireManager, "{1C288214-61CD-4bb9-B44D-21813DCB0017}")

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
			OMEGA_DECLARE_FORWARDS(IWireManager,Omega::System::MetaInfo,IWireManager,Omega,IObject)

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
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
			)
			typedef IWireStub_Impl_Safe<IObject_Safe> IWireStub_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireProxy,

				OMEGA_METHOD_VOID(WriteKey,1,((in),Serialize::IFormattedStream*,pStream))
			)
			typedef IWireProxy_Impl_Safe<IObject_Safe> IWireProxy_Safe;
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireManager,
			
				OMEGA_METHOD_VOID(MarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
				OMEGA_METHOD_VOID(ReleaseMarshalData,3,((in),Serialize::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD_VOID(ReleaseStub,1,((in),Omega::uint32_t,id))
				OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,1,((in),IObject*,pOuter))
				OMEGA_METHOD(IException*,SendAndReceive,4,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pSend,(out),Serialize::IFormattedStream*&,pRecv,(in),uint16_t,timeout))
			)
			typedef IWireManager_Impl_Safe<IObject_Safe> IWireManager_Safe;

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

			template <class T>
			class std_wire_type
			{
			public:
				typedef typename marshal_info<T>::safe_type::type type;

				static IException_Safe* init(type& val)
				{
					val = default_value<type>::value;
					return 0;
				}

				static IException_Safe* read(IWireManager_Safe*, IFormattedStream_Safe* pStream, type& val)
				{
					return wire_read(pStream,val);
				}

				static IException_Safe* write(IWireManager_Safe*, IFormattedStream_Safe* pStream, const type& val)
				{
					return wire_write(pStream,val);
				}

				static IException_Safe* unpack(IWireManager_Safe*, IFormattedStream_Safe* pStream, const type&)
				{
					// Just read the value back, moving the read pointer correctly
					type val = default_value<type>::value();
					return wire_read(pStream,val);
				}

				static IException_Safe* no_op(bool)
				{
					return 0;
				}
			};

			template <class T>
			class std_wire_type<const T>
			{
			public:
				typedef const typename marshal_info<T>::wire_type::type type;

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type& val)
				{
					return marshal_info<T>::wire_type::read(pManager,pStream,const_cast<typename marshal_info<T>::wire_type::type&>(val));
				}

				/*static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type& val, uint32_t cbSize)
				{
					return marshal_info<T>::wire_type::read(pManager,pStream,const_cast<typename marshal_info<T>::wire_type::type&>(val),cbSize);
				}*/

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val)
				{
					return marshal_info<T>::wire_type::write(pManager,pStream,val);
				}

				static IException_Safe* no_op(bool)
				{
					return 0;
				}
			};

			template <class T>
			class std_wire_type<T&>
			{
			public:
				class ref_holder
				{
				public:
					ref_holder(const typename marshal_info<T>::wire_type::type& rhs = default_value<typename marshal_info<T>::wire_type::type>::value()) : m_val(rhs)
					{}

					ref_holder(const ref_holder& rhs) :
						m_val(rhs.m_val)
					{}

					ref_holder& operator = (const ref_holder& rhs)
					{
						m_val = rhs.m_val;
						return *this;
					}

					operator typename marshal_info<T>::wire_type::type*()
					{
						return &m_val;
					}

					typename marshal_info<T>::wire_type::type m_val;
				};
				typedef ref_holder type;

				static IException_Safe* init(type& val)
				{
					return marshal_info<T>::wire_type::init(val.m_val);
				}

				static IException_Safe* init(type& val, const guid_t* piid, IObject* = 0)
				{
					return marshal_info<T>::wire_type::init(val.m_val,piid);
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::read(pManager,pStream,*val);
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::read(pManager,pStream,*val,piid);
				}
				
				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::write(pManager,pStream,*val);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::write(pManager,pStream,*val,piid);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val)
				{
					return marshal_info<T>::wire_type::unpack(pManager,pStream,val.m_val);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::unpack(pManager,pStream,*val);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::unpack(pManager,pStream,*val,piid);
				}

				static IException_Safe* no_op(bool, const guid_t* = 0)
				{
					return 0;
				}
			};

			template <class T>
			class std_wire_type_array
			{
			public:
				class array_holder
				{
				public:
					array_holder() : m_alloc_size(0),m_pVals(0)
					{}

					~array_holder()
					{
						delete [] m_pVals;
					}

					IException_Safe* init(uint32_t cbSize)
					{
						try
						{
							m_alloc_size = cbSize;
							OMEGA_NEW(m_pVals,typename marshal_info<T>::wire_type::type[m_alloc_size]);
						}
						catch (IException* pE)
						{
							return return_safe_exception(pE);
						}
						return 0;
					}

					operator typename marshal_info<T>::wire_type::type*()
					{
						return m_pVals;
					}

					uint32_t                                   m_alloc_size;
					typename marshal_info<T>::wire_type::type* m_pVals;
				};
				typedef array_holder type;

				static IException_Safe* init(type& val, uint32_t* cbSize)
				{
					return val.init(*cbSize);
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					if (pVals)
					{
						for (uint32_t i=0;i<cbSize;++i)
						{
							IException_Safe* pSE = marshal_info<T>::wire_type::read(pManager,pStream,pVals[i]);
							if (pSE)
								return pSE;
						}
					}
					return 0;
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return read(pManager,pStream,pVals,*cbSize);
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type& val, uint32_t cbSize)
				{
					IException_Safe* pSE = val.init(cbSize);
					if (pSE)
						return pSE;

					return read(pManager,pStream,val,&cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					if (pVals)
					{
						for (uint32_t i=0;i<cbSize;++i)
						{
							IException_Safe* pSE = marshal_info<T>::wire_type::write(pManager,pStream,pVals[i]);
							if (pSE)
								return pSE;
						}
					}
					return 0;
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, uint32_t* cbSize)
				{
					return write(pManager,pStream,pVals,*cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, uint32_t cbSize)
				{
					if (cbSize > val.m_alloc_size)
						cbSize = val.m_alloc_size;

					for (uint32_t i=0;i<cbSize;++i)
					{
						IException_Safe* pSE = marshal_info<T>::wire_type::write(pManager,pStream,val.m_pVals[i]);
						if (pSE)
							return pSE;
					}

					return 0;
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, uint32_t* cbSize)
				{
					if (pVals)
					{
						for (uint32_t i=0;i<*cbSize;++i)
						{
							IException_Safe* pSE = marshal_info<T>::wire_type::unpack(pManager,pStream,pVals[i]);
							if (pSE)
								return pSE;
						}
					}
					return 0;
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, uint32_t cbSize)
				{
					if (cbSize > val.m_alloc_size)
						cbSize = val.m_alloc_size;

					for (uint32_t i=0;i<cbSize;++i)
					{
						IException_Safe* pSE = marshal_info<T>::wire_type::unpack(pManager,pStream,val.m_pVals[i]);
						if (pSE)
							return pSE;
					}

					return 0;
				}
				
				static IException_Safe* no_op(bool, uint32_t)
				{
					return 0;
				}

				static IException_Safe* no_op(bool, uint32_t* = 0)
				{
					return 0;
				}
			};

			template <>
			class std_wire_type_array<void>
			{
			public:
				typedef void* type;

				static IException_Safe* read(IWireManager_Safe*, IFormattedStream_Safe*, type&)
				{
					return 0;
				}

				static IException_Safe* write(IWireManager_Safe*, IFormattedStream_Safe*, const type&)
				{
					return 0;
				}
			};

			template <class I>
			class iface_wire_type
			{
			public:
				typedef typename interface_info<I>::safe_class* type;

				static IException_Safe* init(type& val, const guid_t* = 0)
				{
					val = 0;
					return 0;
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type& pI, const guid_t* piid = 0)
				{
					IObject_Safe* p = 0;
					IException_Safe* pSE = pManager->UnmarshalInterface_Safe(pStream,piid ? piid : &OMEGA_UUIDOF(I),&p);
					if (pSE)
						return pSE;
					pI = static_cast<typename interface_info<I>::safe_class*>(p);
					return 0;
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type pI, const guid_t* piid = 0)
				{
					return pManager->MarshalInterface_Safe(pStream,piid ? piid : &OMEGA_UUIDOF(I),pI);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type pI, const guid_t* piid = 0)
				{
					return pManager->ReleaseMarshalData_Safe(pStream,piid ? piid : &OMEGA_UUIDOF(I),pI);
				}

				static IException_Safe* no_op(bool, const guid_t* = 0)
				{
					return 0;
				}
			};

			inline void RegisterWireFactories(const guid_t& iid, void* pfnProxy, void* pfnStub);

			template <class S>
			IWireStub_Safe* CreateWireStub(IWireManager_Safe* pManager, IObject_Safe* pObject)
			{
				S* pS = 0;
				OMEGA_NEW(pS,S(pManager,pObject));
				return pS;
			}

			template <class I>
			class IObject_WireStub : public IWireStub_Safe
			{
			public:
				IObject_WireStub(IWireManager_Safe* pManager, IObject_Safe* pObj) :
					m_pManager(pManager), m_refcount(1)
				{
					m_pS = static_cast<I*>(pObj);
					m_pS->AddRef_Safe();
				}

				virtual void OMEGA_CALL AddRef_Safe()
				{
					// This must do a remote AddRef action
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
					if (*piid == OMEGA_UUIDOF(IObject) || 
						*piid == OMEGA_UUIDOF(IWireStub))
					{
						++m_refcount;
						*retval = this;
					}
					return 0;
				}

				virtual void OMEGA_CALL Pin()
				{
					// This must not do a remote AddRef action
					++m_refcount;
				}

				virtual void OMEGA_CALL Unpin()
				{
					if (--m_refcount==0)
						delete this;
				}

				virtual IException_Safe* OMEGA_CALL SupportsInterface_Safe(bool_t* pbSupports, const guid_t*)
				{
					*pbSupports = false;
					return 0;
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
						return MethodTable[method_id](this,m_pS,pParamsIn,pParamsOut);
					else
						return return_safe_exception(IException::Create(L"Invalid method index"));
				}		
				static const uint32_t MethodCount = 3;

				IWireManager_Safe*  m_pManager;
				I*                  m_pS;

			protected:
				virtual ~IObject_WireStub() 
				{
					m_pS->Release_Safe();
				}

			private:
				System::AtomicOp<uint32_t> m_refcount;
				
				IObject_WireStub(const IObject_WireStub&) {};
				IObject_WireStub& operator =(const IObject_WireStub&) {};

				static IException_Safe* AddRef_Wire(void* pParam,I*,IFormattedStream_Safe*,IFormattedStream_Safe*)
				{
					static_cast<IObject_WireStub<I>*>(pParam)->AddRef_Safe();
					return 0;
				}

				static IException_Safe* Release_Wire(void* pParam,I*,IFormattedStream_Safe*,IFormattedStream_Safe*)
				{
					static_cast<IObject_WireStub<I>*>(pParam)->Release_Safe();
					return 0;
				}

				static IException_Safe* QueryInterface_Wire(void* pParam, I* pI, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					marshal_info<guid_t>::wire_type::type iid;
					
					IException_Safe* pSE = marshal_info<guid_t>::wire_type::read(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsIn,iid);
					if (pSE)
						return pSE;

					IObject_Safe* p;
					pSE = pI->QueryInterface_Safe(&iid,&p);
					if (pSE)
						return pSE;
					auto_iface_safe_ptr<IObject_Safe> ptr(p);

					marshal_info<bool_t&>::wire_type::type bQI = (p != 0);
					return marshal_info<bool_t&>::wire_type::write(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,bQI);
				}
			};

			template <class I_WireProxy>
			class WireProxyImpl : public IObject_Safe
			{
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

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS)
				{
					if (*piid == OMEGA_UUIDOF(IObject))
					{
						*ppS = this;
						(*ppS)->AddRef_Safe();
						return 0;
					}
					return m_contained.Internal_QueryInterface_Safe(false,piid,ppS);
				}

				virtual void OMEGA_CALL Pin()
				{
					m_contained.Pin();
				}

				virtual void OMEGA_CALL Unpin()
				{
					m_contained.Unpin();
				}

				static IObject_Safe* Create(IWireProxy_Safe* pProxy, IWireManager_Safe* pManager)
				{
					IObject_Safe* pRet = 0;
					OMEGA_NEW(pRet,WireProxyImpl(pProxy,pManager));
					return pRet;
				}

			private:
				AtomicOp<uint32_t> m_refcount;
				I_WireProxy        m_contained;

				WireProxyImpl(IWireProxy_Safe* pProxy, IWireManager_Safe* pManager) : 
					m_refcount(1), m_contained(pProxy,pManager)
				{ }

				virtual ~WireProxyImpl()
				{ }
			};			

			template <class Base>
			class IObject_WireProxy : public Base
			{
			public:
				IObject_WireProxy(IWireProxy_Safe* pProxy, IWireManager_Safe* pManager) :
					m_pManager(pManager), m_pProxy(pProxy)
				{
					m_pManager->AddRef_Safe();

					m_pProxy->Pin();
				}

				virtual ~IObject_WireProxy()
				{
					m_pProxy->Unpin();

					m_pManager->Release_Safe();
				}

				virtual void OMEGA_CALL AddRef_Safe()
				{
					m_pProxy->AddRef_Safe();
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					m_pProxy->Release_Safe();
				}

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS)
				{
					return Internal_QueryInterface_Safe(true,piid,ppS);					
				}

				virtual void OMEGA_CALL Pin()
				{
					m_pProxy->Pin();
				}

				virtual void OMEGA_CALL Unpin()
				{
					m_pProxy->Unpin();
				}

				virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppS)
				{
					if (!bRecurse)
					{
						*ppS = 0;
						return 0;
					}
					return m_pProxy->QueryInterface_Safe(piid,ppS);
				}

				static const uint32_t MethodCount = 3;

			protected:
				IWireManager_Safe*  m_pManager;

				IException_Safe* CreateOutputStream(auto_iface_safe_ptr<IFormattedStream_Safe>& pStream)
				{
					IFormattedStream_Safe* p = 0;
					IException_Safe* pSE = m_pManager->CreateOutputStream_Safe(&p,0);
					if (pSE)
						return pSE;
					pStream = p;
					return 0;
				}

				IException_Safe* SendAndReceive(IException_Safe*& pRet, Remoting::MethodAttributes_t attribs, IFormattedStream_Safe* pParamsOut, auto_iface_safe_ptr<IFormattedStream_Safe>& pParamsIn, Omega::uint16_t timeout = 0)
				{
					IFormattedStream_Safe* p = 0;
					IException_Safe* pSE = m_pManager->SendAndReceive_Safe(&pRet,attribs,pParamsOut,&p,timeout);
					if (pSE)
						return pSE;
					pParamsIn = p;
					return 0;
				}

				IException_Safe* WriteKey(IFormattedStream_Safe* pStream, const guid_t& iid)
				{
					IException_Safe* pSE = m_pProxy->WriteKey_Safe(pStream);
					if (pSE)
						return pSE;
					return wire_write(pStream,iid);
				}
				
			private:
				IWireProxy_Safe* m_pProxy;
			};

			OMEGA_QI_MAGIC(Omega::System::MetaInfo,IWireStub)
			OMEGA_QI_MAGIC(Omega::System::MetaInfo,IWireProxy)
			OMEGA_QI_MAGIC(Omega::System::MetaInfo,IWireManager)

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

#endif // OOCORE_WIRE_H_INCLUDED_
