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
			synchronous = 1,
			encrypted = 2
		};
		typedef uint16_t MethodAttributes_t;

		interface IWireManager : public IObject
		{
			virtual void MarshalInterface(Serialize::IFormattedStream* pStream, IObject* pObject, const guid_t& iid) = 0;
			virtual void UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseStub(uint32_t id) = 0;
			virtual Serialize::IFormattedStream* CreateOutputStream() = 0;
			virtual Serialize::IFormattedStream* SendAndReceive(MethodAttributes_t attribs, Serialize::IFormattedStream* pParams) = 0;
		};

		interface IWireStub : public IObject
		{
			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) = 0;
		};

		interface IWireProxy : public IObject
		{
			virtual void WriteKey(Serialize::IFormattedStream* pStream) = 0;
		};
		OMEGA_DECLARE_IID(IWireProxy);
	}

	namespace MetaInfo
	{
		template <class T>
		static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream*, T&);

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, byte_t& val)
		{
			val = pStream->ReadByte();
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, bool_t& val)
		{
			val = pStream->ReadBoolean();
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, uint16_t& val)
		{
			val = pStream->ReadUInt16();
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, uint32_t& val)
		{
			val = pStream->ReadUInt32();
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, uint64_t& val)
		{
			val = pStream->ReadUInt64();
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, guid_t& val)
		{
			val.Data1 = pStream->ReadUInt32();
			val.Data2 = pStream->ReadUInt16();
			val.Data3 = pStream->ReadUInt16();
			uint32_t bytes = 8;
			pStream->ReadBytes(bytes,val.Data4);
			if (bytes != 8)
				OMEGA_THROW("Failed to read guid_t");
		}

		template <>
		inline static void wire_read(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, string_t& val)
		{
			val = pStream->ReadString();
		}

		template <class I>
		inline static void wire_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, I*& val, const guid_t iid = iid_traits<I>::GetIID())
		{
			IObject* pObject = 0;
			pManager->UnmarshalInterface(pStream,iid,pObject);
			val = static_cast<I*>(pObject);
		}

		template <class T>
		static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream*, const T&);

		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, byte_t val)
		{
			pStream->WriteByte(val);
		}

		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, bool_t val)
		{
			pStream->WriteBoolean(val);
		}

		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, uint16_t val)
		{
			pStream->WriteUInt16(val);
		}

		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, uint32_t val)
		{
			pStream->WriteUInt32(val);
		}

		template <>
		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, const uint64_t& val)
		{
			pStream->WriteUInt64(val);
		}

		template <>
		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, const guid_t& val)
		{
			pStream->WriteUInt32(val.Data1);
			pStream->WriteUInt16(val.Data2);
			pStream->WriteUInt16(val.Data3);
			pStream->WriteBytes(8,val.Data4);
		}

		template <>
		inline static void wire_write(Remoting::IWireManager*, Serialize::IFormattedStream* pStream, const string_t& val)
		{
			pStream->WriteString(val);
		}

		template <class I>
		inline static void wire_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, I* val, const guid_t iid = iid_traits<I>::GetIID())
		{
			pManager->MarshalInterface(pStream,val,iid);
		}

		template <class T>
		struct std_wire_type
		{
			std_wire_type(Remoting::IWireManager* = 0) : m_val(m_fixed)
			{}

			std_wire_type(const T& val) : m_fixed(val), m_val(m_fixed)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream) : m_val(m_fixed)
			{
				read(pManager,pStream);	
			}

			std_wire_type& operator = (const T& val)
			{
				m_val = val;
				return *this;
			}

			void attach(T& val_ref)
			{
				m_val = val_ref;
			}

			void read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{				
				wire_read(pManager,pStream,m_val);
			}

			static void proxy_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T& val)
			{
				wire_read(pManager,pStream,val);
			}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*)
			{}

			void write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				wire_write(pManager,pStream,m_val);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const T& val)
			{
				wire_write(pManager,pStream,val);
			}

			operator T&()
			{
				return m_val;
			}

		private:
			T	m_fixed;
			T&	m_val;

			std_wire_type(const std_wire_type&) {}
			std_wire_type& operator = (const std_wire_type&) {}
		};

		template <class T>
		struct std_wire_type<const T>
		{
			std_wire_type(Remoting::IWireManager* pManager = 0) : m_actual(pManager)
			{}

			std_wire_type(Remoting::IWireManager* pManager, const guid_t& iid) : m_actual(pManager,iid)
			{}

			std_wire_type(const T& val) : m_actual(val)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream) :
				m_actual(pManager,pStream)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize) :
				m_actual(pManager,pStream,cbSize)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid) :
				m_actual(pManager,pStream,iid)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize) :
				m_actual(pManager,pStream,iid,cbSize)
			{}

			std_wire_type& operator = (const T& val)
			{
				m_actual = val;
				return *this;
			}

			void attach(const T& val_ref)
			{
				m_actual.attach(const_cast<T&>(val_ref));
			}

			void read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				m_actual.read(pManager,pStream);
			}

			static void proxy_read(Remoting::IWireManager*, Serialize::IFormattedStream*, const T&)
			{}

			static void proxy_read(Remoting::IWireManager*, Serialize::IFormattedStream*, const T&, uint32_t)
			{}

			static void proxy_read(Remoting::IWireManager*, Serialize::IFormattedStream*, const T&, const guid_t&)
			{}

			static void proxy_read(Remoting::IWireManager*, Serialize::IFormattedStream*, const T&, const guid_t&, uint32_t)
			{}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*)
			{}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*, uint32_t)
			{}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*, const guid_t&)
			{}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*, const guid_t&, uint32_t)
			{}

			void write(Remoting::IWireManager*, Serialize::IFormattedStream*)
			{}

			void write(Remoting::IWireManager*, Serialize::IFormattedStream*, uint32_t)
			{}

			void write(Remoting::IWireManager*, Serialize::IFormattedStream*, const guid_t&)
			{}

			void write(Remoting::IWireManager*, Serialize::IFormattedStream*, const guid_t&, uint32_t)
			{}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const T& pVal)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,pVal);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const T& pVal, uint32_t cbSize)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,pVal,cbSize);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const T& pVal, const guid_t& iid)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,pVal,iid);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const T& pVal, const guid_t& iid, uint32_t cbSize)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,pVal,iid,cbSize);
			}

			operator const T&()
			{
				return m_actual;
			}

		private:
			typename interface_info<T>::wire_type	m_actual;

			std_wire_type(const std_wire_type&) {}
			std_wire_type& operator = (const std_wire_type&) {}
		};

		template <class T>
		struct std_wire_type<T&>
		{
			std_wire_type(Remoting::IWireManager* pManager) : m_actual(pManager)
			{}

			std_wire_type(Remoting::IWireManager* pManager, const guid_t& iid) : m_actual(pManager,iid)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream) :
				m_actual(pManager,pStream)
			{}

			std_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid) :
				m_actual(pManager,pStream,iid)
			{}

			static void proxy_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T& val)
			{
				interface_info<T>::wire_type::proxy_read(pManager,pStream,val);
			}

			static void proxy_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T& val, const guid_t& iid)
			{
				interface_info<T>::wire_type::proxy_read(pManager,pStream,val,iid);
			}

			void out(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream)
			{
				m_actual.write(pManager,pStream);
			}

			void out(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid)
			{
				m_actual.write(pManager,pStream,iid);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T& val)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,val);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T& val, const guid_t& iid)
			{
				interface_info<T>::wire_type::proxy_write(pManager,pStream,val,iid);
			}

			operator T& ()
			{
				return m_actual;
			}

		private:
			typename interface_info<T>::wire_type	m_actual;

			std_wire_type(const std_wire_type&) {}
			std_wire_type& operator = (const std_wire_type&) {}
		};

		template <class T>
		struct std_wire_type_array
		{
			std_wire_type_array(Remoting::IWireManager*, uint32_t cbSize = 1) : 
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

			std_wire_type_array(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize = 1) :
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

			std_wire_type_array(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize = 1) :
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

			std_wire_type_array(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t* piids, uint32_t cbSize = 1) :
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

			~std_wire_type_array()
			{
				delete [] m_pFunctors;
				delete [] m_pVals;
			}

			static void proxy_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T* pVals, uint32_t cbSize = 1)
			{
				for (uint32_t i=0;i<cbSize;++i)
					interface_info<T>::wire_type::proxy_read(pManager,pStream,pVals[i]);
			}

			void out(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream);
			}

			void out(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream,iid);
			}

			void out(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t* piids, uint32_t cbSize = 1)
			{
				if (cbSize > m_alloc_size)
					OMEGA_THROW("Array has been resized out of bounds");

				for (uint32_t i=0;i<cbSize;++i)
					m_pFunctors[i].write(pManager,pStream,piids[i]);
			}

            static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, T* pVals, uint32_t cbSize = 1)
			{
				for (uint32_t i=0;i<cbSize;++i)
					interface_info<T>::wire_type::proxy_write(pManager,pStream,pVals[i]);
			}

			operator T*()
			{
				return m_pVals;
			}

		private:
			typename interface_info<T>::wire_type* m_pFunctors;
			T*                                     m_pVals;
			const uint32_t                         m_alloc_size;

			void init(uint32_t cbSize)
			{
				if (cbSize > 0)
				{
					OMEGA_NEW(m_pFunctors,interface_info<T>::wire_type[cbSize]);
					OMEGA_NEW(m_pVals,T[cbSize]);

					for (uint32_t i=0;i<cbSize;++i)
						m_pFunctors[i].attach(m_pVals[i]);
				}
			}

			std_wire_type_array(const std_wire_type_array&) {};
			std_wire_type_array& operator = (const std_wire_type_array&) {};
		};

		template <class I>
		struct iface_wire_type<I*>
		{
			iface_wire_type(Remoting::IWireManager*, const guid_t& = iid_traits<I>::GetIID()) :
				m_fixed(0), m_pI(m_fixed)
			{}

			iface_wire_type(I* pI) :
				m_fixed(pI), m_pI(m_fixed)
			{
				if (m_pI)
					m_pI->AddRef();
			}

			iface_wire_type(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID()) :
				m_fixed(0), m_pI(m_fixed)
			{
				read(pManager,pStream,iid);
			}

			iface_wire_type& operator = (I* val)
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

			virtual ~iface_wire_type()
			{
				if (m_pI)
					m_pI->Release();
			}

			void attach(I*& val_ref)
			{
				m_pI = val_ref;
			}

			void read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_read(pManager,pStream,m_pI,iid);
			}

			static void proxy_read(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, I*& pI, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_read(pManager,pStream,pI,iid);
			}

			void out(Remoting::IWireManager*, Serialize::IFormattedStream*, const guid_t& = iid_traits<I>::GetIID())
			{ }

			void write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_write(pManager,pStream,m_pI,iid);
			}

			static void proxy_write(Remoting::IWireManager* pManager, Serialize::IFormattedStream* pStream, I* pI, const guid_t& iid = iid_traits<I>::GetIID())
			{
				wire_write(pManager,pStream,pI,iid);
			}

			operator I*&()
			{
				return m_pI;
			}

		private:
			I*				m_fixed;
			I*&				m_pI;

			iface_wire_type(const iface_wire_type&) {}
			iface_wire_type& operator = (const iface_wire_type&) {}
		};

		template <class I_WireProxy, class I>
		class WireProxyImpl : public IObject
		{
			struct Contained : public I_WireProxy
			{
				Contained(IObject* pOuter, Remoting::IWireManager* pManager) : 
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
				
				IObject* m_pOuter;
			};
			Contained					m_contained;
			AtomicOp<uint32_t>::type	m_refcount;

			virtual ~WireProxyImpl()
			{}

		public:
			WireProxyImpl(IObject* pOuter, Remoting::IWireManager* pManager) : m_contained(pOuter,pManager), m_refcount(1)
			{ }

			static IObject* Create(IObject* pOuter, Remoting::IWireManager* pManager)
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
		static Remoting::IWireStub* CreateWireStub(Remoting::IWireManager* pManager, IObject* pObject, uint32_t id)
		{
			I* pI;
			OMEGA_NEW(pI,I(pManager,pObject,id));
			return pI;
		}

		template <class I>
		struct IObject_WireStub : public Remoting::IWireStub
		{
			IObject_WireStub(Remoting::IWireManager* pManager, IObject* pObj, uint32_t id) : 
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
				interface_info<const guid_t&>::wire_type iid(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsIn);
				iface_wire_type<IObject*> retval = pI->QueryInterface(iid);
				retval.out(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,iid);
			}

			Remoting::IWireManager* m_pManager;
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
			IObject_WireProxy(Remoting::IWireManager* pManager) : m_pManager(pManager)
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
				Remoting::IWireProxy* pProxy = static_cast<Remoting::IWireProxy*>(this->QueryInterface(Remoting::IID_IWireProxy));
				if (!pProxy)
					INoInterfaceException::Throw(Remoting::IID_IWireProxy,OMEGA_FUNCNAME);

				try
				{
					pProxy->WriteKey(pParams);
				}
				catch (...)
				{
					pProxy->Release();
					throw;
				}
				pProxy->Release();
			}

			static const uint32_t MethodCount = 3;

		protected:
			Remoting::IWireManager* m_pManager;
		};

		template <class I, class Base>
		struct IException_WireStub : public Base
		{
			IException_WireStub(Remoting::IWireManager* pManager, IObject* pObj, uint32_t id) : Base(pManager,pObj,id)
			{}

			virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout)
			{
				static const MethodTableEntry MethodTable[] =
				{
					ActualIID_Wire,
					Cause_Wire,
					Description_Wire,
					Source_Wire
				};

				if (method_id < Base::MethodCount)
					Base::Invoke(method_id,pParamsIn,pParamsOut,timeout);
				else if (method_id < MethodCount)
					MethodTable[method_id - Base::MethodCount](this,m_pI,pParamsIn,pParamsOut);
				else
					OMEGA_THROW("Invalid method index");
			}
			static const uint32_t MethodCount = Base::MethodCount + 4;

			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(guid_t,ActualIID,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(IException*,Cause,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(string_t,Description,0,())
			OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(string_t,Source,0,())

		private:
			IException_WireStub() {};
			IException_WireStub(const IException_WireStub&) {};
			IException_WireStub& operator =(const IException_WireStub&) {};
		};

		template <class I, class Base>
		struct IException_WireProxy : public Base
		{
			IException_WireProxy(Remoting::IWireManager* pManager) : Base(pManager)
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

			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(guid_t,ActualIID,0,()) 0;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(IException*,Cause,0,()) 1;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(string_t,Description,0,()) 2;
			OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(string_t,Source,0,()) 3;

			static const uint32_t MethodCount = Base::MethodCount + 4;
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

OMEGA_DEFINE_IID(Omega::Remoting,IWireProxy,0xe52b96ce, 0xe4ac, 0x4327, 0x91, 0x17, 0x48, 0x68, 0x4c, 0x8a, 0x38, 0x0);

#endif // OOCORE_WIRE_H_INCLUDED_

