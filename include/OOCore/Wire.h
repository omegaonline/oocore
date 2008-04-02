///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_WIRE_H_INCLUDED_
#define OOCORE_WIRE_H_INCLUDED_

namespace Omega
{
	namespace IO
	{
		interface IStream : public IObject
		{
			virtual void ReadBytes(uint32_t& cbBytes, byte_t* val) = 0;
			virtual uint32_t WriteBytes(uint32_t cbBytes, const byte_t* val) = 0;
		};

		interface IFormattedStream : public IStream
		{
			virtual bool_t ReadBoolean() = 0;
			virtual byte_t ReadByte() = 0;
			virtual int16_t ReadInt16() = 0;
			virtual uint16_t ReadUInt16() = 0;
			virtual int32_t ReadInt32() = 0;
			virtual uint32_t ReadUInt32() = 0;
			virtual int64_t ReadInt64() = 0;
			virtual uint64_t ReadUInt64() = 0;
			virtual string_t ReadString() = 0;
			virtual guid_t ReadGuid() = 0;

			virtual void WriteBoolean(bool_t val) = 0;
			virtual void WriteByte(byte_t val) = 0;
			virtual void WriteInt16(int16_t val) = 0;
			virtual void WriteUInt16(uint16_t val) = 0;
			virtual void WriteInt32(int32_t val) = 0;
			virtual void WriteUInt32(uint32_t val) = 0;
			virtual void WriteInt64(const int64_t& val) = 0;
			virtual void WriteUInt64(const uint64_t& val) = 0;
			virtual void WriteString(const string_t& val) = 0;
			virtual void WriteGuid(const guid_t& val) = 0;
		};
	}

	namespace Remoting
	{
		enum MethodAttributes
		{
			synchronous = 0,
			asynchronous = 1,
			unreliable = 2,
			encrypted = 4
		};
		typedef uint16_t MethodAttributes_t;
	}

	namespace System
	{
		namespace MetaInfo
		{
			interface IWireManager : public IObject
			{
				virtual void MarshalInterface(IO::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
				virtual void UnmarshalInterface(IO::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject) = 0;
				virtual void ReleaseMarshalData(IO::IFormattedStream* pStream, const guid_t& iid, IObject* pObject) = 0;
				virtual IO::IFormattedStream* CreateOutputStream() = 0;
				virtual IException* SendAndReceive(Remoting::MethodAttributes_t attribs, IO::IFormattedStream* pSend, IO::IFormattedStream*& pRecv, uint16_t timeout = 0) = 0;
			};

			interface IWireStub : public IObject
			{
				virtual void Invoke(IO::IFormattedStream* pParamsIn, IO::IFormattedStream* pParamsOut) = 0;
				virtual bool_t SupportsInterface(const guid_t& iid) = 0;
			};

			interface IWireStubController : public IObject
			{
				virtual void RemoteRelease(uint32_t release_count) = 0;
				virtual bool_t SupportsInterface(const guid_t& iid) = 0;
				virtual void MarshalStub(IO::IFormattedStream* pParamsIn, IO::IFormattedStream* pParamsOut) = 0;
			};

			interface IWireProxy : public IObject
			{
				virtual void WriteKey(IO::IFormattedStream* pStream) = 0;
				virtual bool_t IsAlive() = 0;
			};
		}
	}
}

OMEGA_DEFINE_IID(Omega::IO, IStream, "{D1072F9B-3E7C-4724-9246-46DC111AE69F}")
OMEGA_DEFINE_IID(Omega::IO, IFormattedStream, "{044E0896-8A60-49e8-9143-5B1F01D4AE4C}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireStub, "{0785F8A6-A6BE-4714-A306-D9886128A40E}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireStubController, "{B9AD6795-72FA-45a4-9B91-68CE1D5B6283}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireProxy, "{0D4BE871-5AD0-497b-A018-EDEA8C17255B}")
OMEGA_DEFINE_IID(Omega::System::MetaInfo, IWireManager, "{1C288214-61CD-4bb9-B44D-21813DCB0017}")

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			OMEGA_DECLARE_FORWARDS(IStream,Omega::IO,IStream,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IFormattedStream,Omega::IO,IFormattedStream,Omega::IO,IStream)
			OMEGA_DECLARE_FORWARDS(IWireStub,Omega::System::MetaInfo,IWireStub,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IWireStubController,Omega::System::MetaInfo,IWireStubController,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IWireProxy,Omega::System::MetaInfo,IWireProxy,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IWireManager,Omega::System::MetaInfo,IWireManager,Omega,IObject)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::IO, IStream,

				OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
				OMEGA_METHOD(uint32_t,WriteBytes,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::IO, IFormattedStream,

				OMEGA_METHOD(bool_t,ReadBoolean,0,())
				OMEGA_METHOD(byte_t,ReadByte,0,())
				OMEGA_METHOD(int16_t,ReadInt16,0,())
				OMEGA_METHOD(uint16_t,ReadUInt16,0,())
				OMEGA_METHOD(int32_t,ReadInt32,0,())
				OMEGA_METHOD(uint32_t,ReadUInt32,0,())
				OMEGA_METHOD(int64_t,ReadInt64,0,())
				OMEGA_METHOD(uint64_t,ReadUInt64,0,())
				OMEGA_METHOD(string_t,ReadString,0,())
				OMEGA_METHOD(guid_t,ReadGuid,0,())

				OMEGA_METHOD_VOID(WriteBoolean,1,((in),bool_t,val))
				OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
				OMEGA_METHOD_VOID(WriteInt16,1,((in),int16_t,val))
				OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
				OMEGA_METHOD_VOID(WriteInt32,1,((in),int32_t,val))
				OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
				OMEGA_METHOD_VOID(WriteInt64,1,((in),const int64_t&,val))
				OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
				OMEGA_METHOD_VOID(WriteString,1,((in),const string_t&,val))
				OMEGA_METHOD_VOID(WriteGuid,1,((in),const guid_t&,val))
			)
			typedef IFormattedStream_Impl_Safe<interface_info<IO::IStream>::safe_class> IFormattedStream_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireStub,

				OMEGA_METHOD_VOID(Invoke,2,((in),IO::IFormattedStream*,pParamsIn,(in),IO::IFormattedStream*,pParamsOut))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
			)
			typedef IWireStub_Impl_Safe<IObject_Safe> IWireStub_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireStubController,

				OMEGA_METHOD_VOID(RemoteRelease,1,((in),uint32_t,release_count))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD_VOID(MarshalStub,2,((in),IO::IFormattedStream*,pParamsIn,(in),IO::IFormattedStream*,pParamsOut))
			)
			typedef IWireStubController_Impl_Safe<IObject_Safe> IWireStubController_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireProxy,

				OMEGA_METHOD_VOID(WriteKey,1,((in),IO::IFormattedStream*,pStream))
				OMEGA_METHOD(bool_t,IsAlive,0,())
			)
			typedef IWireProxy_Impl_Safe<IObject_Safe> IWireProxy_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System::MetaInfo, IWireManager,

				OMEGA_METHOD_VOID(MarshalInterface,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD_VOID(UnmarshalInterface,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
				OMEGA_METHOD_VOID(ReleaseMarshalData,3,((in),IO::IFormattedStream*,pStream,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD(IO::IFormattedStream*,CreateOutputStream,0,())
				OMEGA_METHOD(IException*,SendAndReceive,4,((in),Remoting::MethodAttributes_t,attribs,(in),IO::IFormattedStream*,pSend,(out),IO::IFormattedStream*&,pRecv,(in),uint16_t,timeout))
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

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, int16_t& val)
			{
				return pStream->ReadInt16_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint16_t& val)
			{
				return pStream->ReadUInt16_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, int32_t& val)
			{
				return pStream->ReadInt32_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint32_t& val)
			{
				return pStream->ReadUInt32_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, int64_t& val)
			{
				return pStream->ReadInt64_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, uint64_t& val)
			{
				return pStream->ReadUInt64_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, guid_t& val)
			{
				return pStream->ReadGuid_Safe(&val);
			}

			static IException_Safe* wire_read(IFormattedStream_Safe* pStream, string_t& val)
			{
				return pStream->ReadString_Safe(&val);
			}

			template <class T>
			static IException_Safe* wire_read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, T* pVals, uint32_t cbMaxSize)
			{
				if (!pVals)
					cbMaxSize = 0;

				uint32_t cbSize = 0;
				IException_Safe* pSE = wire_read(pStream,cbSize);
				if (pSE)
					return pSE;

				if (cbSize > cbMaxSize)
					cbSize = cbMaxSize;

				for (uint32_t i=0;i<cbSize;++i)
				{
					pSE = marshal_info<T>::wire_type::read(pManager,pStream,pVals[i]);
					if (pSE)
						return pSE;
				}
				
				return 0;
			}

			static IException_Safe* wire_read(IWireManager_Safe*, IFormattedStream_Safe* pStream, byte_t* pVals, uint32_t cbMaxSize)
			{
				if (!pVals)
					cbMaxSize = 0;

				uint32_t cbSize = 0;
				IException_Safe* pSE = wire_read(pStream,cbSize);
				if (pSE)
					return pSE;

				if (cbSize > cbMaxSize)
					cbSize = cbMaxSize;

				if (!cbSize)
					return 0;

				uint32_t i = cbSize;
				pSE = pStream->ReadBytes_Safe(&i,pVals);
				if (pSE)
					return pSE;
				if (i != cbSize)
					return return_safe_exception(ISystemException::Create(EIO));

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

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, int16_t val)
			{
				return pStream->WriteInt16_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, uint16_t val)
			{
				return pStream->WriteUInt16_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, int32_t val)
			{
				return pStream->WriteInt32_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, uint32_t val)
			{
				return pStream->WriteUInt32_Safe(val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const int64_t& val)
			{
				return pStream->WriteInt64_Safe(&val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const uint64_t& val)
			{
				return pStream->WriteUInt64_Safe(&val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const guid_t& val)
			{
				return pStream->WriteGuid_Safe(&val);
			}

			static IException_Safe* wire_write(IFormattedStream_Safe* pStream, const string_t& val)
			{
				return pStream->WriteString_Safe(&val);
			}

			template <class T>
			static IException_Safe* wire_write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const T* pVals, uint32_t cbSize)
			{
				if (!pVals)
					cbSize = 0;

				IException_Safe* pSE = wire_write(pStream,cbSize);
				if (pSE)
					return pSE;
				
				for (uint32_t i=0;i<cbSize;++i)
				{
					pSE = marshal_info<T>::wire_type::write(pManager,pStream,pVals[i]);
					if (pSE)
						return pSE;
				}
				
				return 0;
			}

			static IException_Safe* wire_write(IWireManager_Safe*, IFormattedStream_Safe* pStream, const byte_t* pVals, uint32_t cbSize)
			{
				if (!pVals)
					cbSize = 0;

				IException_Safe* pSE = wire_write(pStream,cbSize);
				if (pSE)
					return pSE;

				if (!cbSize)
					return 0;

				byte_t* p = const_cast<byte_t*>(pVals);
				while (cbSize)
				{
					uint32_t cb = 0;
					pSE = pStream->WriteBytes_Safe(&cb,cbSize,p);
					if (pSE)
						return pSE;

					p += cb;
					cbSize -= cb;
				}
					
				return 0;
			}

			template <class T>
			class std_wire_type
			{
			public:
				typedef typename marshal_info<T>::safe_type::type type;
				typedef typename marshal_info<T>::safe_type::type real_type;

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
				typedef const typename marshal_info<T>::wire_type::type real_type;

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, type& val)
				{
					return marshal_info<T>::wire_type::read(pManager,pStream,const_cast<typename marshal_info<T>::wire_type::type&>(val));
				}

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

					operator typename marshal_info<T>::wire_type::real_type*()
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
							if (cbSize > (size_t)-1 / sizeof(typename marshal_info<typename remove_const<T>::type>::wire_type::type))
								OMEGA_THROW(E2BIG);

							m_alloc_size = cbSize;
							OMEGA_NEW(m_pVals,typename marshal_info<typename remove_const<T>::type>::wire_type::type[m_alloc_size]);
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

					uint32_t                                                                m_alloc_size;
					typename marshal_info<typename remove_const<T>::type>::wire_type::type* m_pVals;
				};
				typedef array_holder type;

				static IException_Safe* init(type& val, const uint32_t* cbSize)
				{
					return val.init(*cbSize);
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					return wire_read(pManager,pStream,pVals,cbSize);
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

					return read(pManager,pStream,val.m_pVals,cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					return wire_write(pManager,pStream,pVals,cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return write(pManager,pStream,pVals,*cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, uint32_t cbSize)
				{
					// Only write back what we have room for...
					if (cbSize > val.m_alloc_size)
						cbSize = val.m_alloc_size;

					return wire_write(pManager,pStream,val.m_pVals,cbSize);
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, const uint32_t* cbSize)
				{
					return write(pManager,pStream,val,*cbSize);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return wire_read(pManager,pStream,pVals,*cbSize);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, uint32_t cbSize)
				{
					// Only read what we have room for...
					if (cbSize > val.m_alloc_size)
						cbSize = val.m_alloc_size;

					return wire_read(pManager,pStream,val.m_pVals,cbSize);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, const type& val, const uint32_t* cbSize)
				{
					return unpack(pManager,pStream,val,*cbSize);
				}

				static IException_Safe* no_op(bool, uint32_t)
				{
					return 0;
				}

				static IException_Safe* no_op(bool, const uint32_t* = 0)
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
					return return_safe_exception(ISystemException::Create(EINVAL));
				}

				static IException_Safe* write(IWireManager_Safe*, IFormattedStream_Safe*, const type&)
				{
					return return_safe_exception(ISystemException::Create(EINVAL));
				}
			};

			template <class I>
			class iface_wire_type
			{
			public:
				typedef typename interface_info<I>::safe_class* real_type;

				class if_holder
				{
				public:
					if_holder() : m_val(0)
					{ }

					if_holder(const if_holder& rhs) : m_val(rhs.m_val)
					{
						if (m_val)
							m_val->AddRef_Safe();
					}

					if_holder& operator = (const if_holder& rhs)
					{
						if (&rhs != this)
						{
							if (m_val)
								m_val->Release_Safe();

							m_val = rhs.m_val;
							if (m_val)
								m_val->AddRef_Safe();
						}
						return *this;
					}

					~if_holder()
					{
						if (m_val)
							m_val->Release_Safe();
					}

					operator real_type&()
					{
						return m_val;
					}

					real_type* operator &()
					{
						return &m_val;
					}

					real_type operator ->()
					{
						return m_val;
					}

				private:
					real_type m_val;
				};
				typedef if_holder type;

				static IException_Safe* init(type&, const guid_t* = 0)
				{
					return 0;
				}

				static IException_Safe* read(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, real_type& pI, const guid_t* piid = 0)
				{
					IObject_Safe* p = 0;
					IException_Safe* pSE = pManager->UnmarshalInterface_Safe(pStream,piid ? piid : &OMEGA_UUIDOF(I),&p);
					if (pSE)
						return pSE;
					pI = static_cast<real_type>(p);
					return 0;
				}

				static IException_Safe* write(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, real_type pI, const guid_t* piid = 0)
				{
					return pManager->MarshalInterface_Safe(pStream,piid ? piid : &OMEGA_UUIDOF(I),pI);
				}

				static IException_Safe* unpack(IWireManager_Safe* pManager, IFormattedStream_Safe* pStream, real_type pI, const guid_t* piid = 0)
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
			IWireStub_Safe* CreateWireStub(IWireStubController_Safe* pController, IWireManager_Safe* pManager, IObject_Safe* pObject)
			{
				S* pS = 0;
				OMEGA_NEW(pS,S(pController,pManager,pObject));
				return pS;
			}

			template <class I>
			class IObject_WireStub : public IWireStub_Safe
			{
			public:
				IObject_WireStub(IWireStubController_Safe* pController, IWireManager_Safe* pManager, IObject_Safe* pObj) :
					m_pManager(pManager), m_refcount(1), m_pController(pController)
				{
					m_pS = static_cast<I*>(pObj);
					m_pS->AddRef_Safe();
				}

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
					if (*piid == OMEGA_UUIDOF(IObject) ||
						*piid == OMEGA_UUIDOF(IWireStub))
					{
						++m_refcount;
						*retval = this;
					}
					return 0;
				}

				// These will never be called
				virtual void OMEGA_CALL Pin() {}
				virtual void OMEGA_CALL Unpin() {}

				virtual IException_Safe* OMEGA_CALL SupportsInterface_Safe(bool_t* pbSupports, const guid_t*)
				{
					*pbSupports = false;
					return 0;
				}

				typedef IException_Safe* (*MethodTableEntry)(void* pParam, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut);

				virtual IException_Safe* OMEGA_CALL Invoke_Safe(IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					// Read the method id
					uint32_t method_id = 0;
					IException_Safe* pSE = pParamsIn->ReadUInt32_Safe(&method_id);
					if (pSE)
						return pSE;

					return Internal_Invoke_Safe(method_id,pParamsIn,pParamsOut);
				}

				virtual IException_Safe* Internal_Invoke_Safe(uint32_t method_id, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					static const MethodTableEntry MethodTable[] =
					{
						RemoteRelease_Wire,
						QueryInterface_Wire,
						MarshalStub_Wire
					};

					if (method_id < MethodCount)
						return MethodTable[method_id](this,pParamsIn,pParamsOut);
					else
						return return_safe_exception(ISystemException::Create(EINVAL));
				}
				static const uint32_t MethodCount = 3; // This must match IObject_WireProxy

				IWireManager_Safe*  m_pManager;
				I*                  m_pS;

			protected:
				virtual ~IObject_WireStub()
				{
					m_pS->Release_Safe();
				}

			private:
				System::AtomicOp<uint32_t> m_refcount;
				IWireStubController_Safe*  m_pController;

				IObject_WireStub(const IObject_WireStub&) {};
				IObject_WireStub& operator =(const IObject_WireStub&) {};

				static IException_Safe* RemoteRelease_Wire(void* pParam, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe*)
				{
					uint32_t release_count = 0;
					IException_Safe* pSE = pParamsIn->ReadUInt32_Safe(&release_count);
					if (pSE)
						return pSE;

					return static_cast<IObject_WireStub<I>*>(pParam)->m_pController->RemoteRelease_Safe(release_count);
				}

				static IException_Safe* QueryInterface_Wire(void* pParam, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					marshal_info<guid_t>::wire_type::type iid;
					IException_Safe* pSE = marshal_info<guid_t>::wire_type::read(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsIn,iid);
					if (pSE)
						return pSE;

					marshal_info<bool_t&>::wire_type::type bQI = false;
					pSE = static_cast<IObject_WireStub<I>*>(pParam)->m_pController->SupportsInterface_Safe(bQI,&iid);
					if (pSE)
						return pSE;

					return marshal_info<bool_t&>::wire_type::write(static_cast<IObject_WireStub<I>*>(pParam)->m_pManager,pParamsOut,bQI);
				}

				static IException_Safe* MarshalStub_Wire(void* pParam, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut)
				{
					return static_cast<IObject_WireStub<I>*>(pParam)->m_pController->MarshalStub_Safe(pParamsIn,pParamsOut);
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
				}

				virtual ~IObject_WireProxy()
				{
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

				static const uint32_t MethodCount = 3;	// This must match IObject_WireStub

			protected:
				IWireManager_Safe*  m_pManager;

				IException_Safe* CreateOutputStream(auto_iface_safe_ptr<IFormattedStream_Safe>& pStream)
				{
					IFormattedStream_Safe* p = 0;
					IException_Safe* pSE = m_pManager->CreateOutputStream_Safe(&p);
					if (pSE)
						return pSE;
					pStream.attach(p);
					return 0;
				}

				IException_Safe* SendAndReceive(IException_Safe*& pRet, Remoting::MethodAttributes_t attribs, IFormattedStream_Safe* pParamsOut, auto_iface_safe_ptr<IFormattedStream_Safe>& pParamsIn, uint16_t timeout = 0)
				{
					IFormattedStream_Safe* p = 0;
					IException_Safe* pSE = m_pManager->SendAndReceive_Safe(&pRet,attribs,pParamsOut,&p,timeout);
					if (pSE)
						return pSE;
					pParamsIn.attach(p);
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
			OMEGA_QI_MAGIC(Omega::System::MetaInfo,IWireStubController)
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
				Omega::IO, IStream,

				OMEGA_METHOD_VOID(ReadBytes,2,((in_out),uint32_t&,cbBytes,(out)(size_is(cbBytes)),byte_t*,val))
				OMEGA_METHOD(uint32_t,WriteBytes,2,((in),uint32_t,cbBytes,(in)(size_is(cbBytes)),const byte_t*,val))
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega::IO, IFormattedStream,

				OMEGA_METHOD(bool_t,ReadBoolean,0,())
				OMEGA_METHOD(byte_t,ReadByte,0,())
				OMEGA_METHOD(int16_t,ReadInt16,0,())
				OMEGA_METHOD(uint16_t,ReadUInt16,0,())
				OMEGA_METHOD(int32_t,ReadInt32,0,())
				OMEGA_METHOD(uint32_t,ReadUInt32,0,())
				OMEGA_METHOD(int64_t,ReadInt64,0,())
				OMEGA_METHOD(uint64_t,ReadUInt64,0,())
				OMEGA_METHOD(string_t,ReadString,0,())
				OMEGA_METHOD(guid_t,ReadGuid,0,())

				OMEGA_METHOD_VOID(WriteBoolean,1,((in),bool_t,val))
				OMEGA_METHOD_VOID(WriteByte,1,((in),byte_t,val))
				OMEGA_METHOD_VOID(WriteInt16,1,((in),int16_t,val))
				OMEGA_METHOD_VOID(WriteUInt16,1,((in),uint16_t,val))
				OMEGA_METHOD_VOID(WriteInt32,1,((in),int32_t,val))
				OMEGA_METHOD_VOID(WriteUInt32,1,((in),uint32_t,val))
				OMEGA_METHOD_VOID(WriteInt64,1,((in),const int64_t&,val))
				OMEGA_METHOD_VOID(WriteUInt64,1,((in),const uint64_t&,val))
				OMEGA_METHOD_VOID(WriteString,1,((in),const string_t&,val))
				OMEGA_METHOD_VOID(WriteGuid,1,((in),const guid_t&,val))
			)
		}
	}
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const Omega::guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub));
void Omega::System::MetaInfo::RegisterWireFactories(const guid_t& iid, void* pfnProxy, void* pfnStub)
{
	Omega_RegisterWireFactories(iid,pfnProxy,pfnStub);
}

#endif // OOCORE_WIRE_H_INCLUDED_
