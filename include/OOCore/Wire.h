///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
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
	namespace Remoting
	{
		interface IMessage : public IObject
		{
			virtual size_t ReadBooleans(const wchar_t* pszName, size_t count, bool_t* arr) = 0;
			virtual size_t ReadBytes(const wchar_t* pszName, size_t count, byte_t* arr) = 0;
			virtual size_t ReadInt16s(const wchar_t* pszName, size_t count, int16_t* arr) = 0;
			virtual size_t ReadUInt16s(const wchar_t* pszName, size_t count, uint16_t* arr) = 0;
			virtual size_t ReadInt32s(const wchar_t* pszName, size_t count, int32_t* arr) = 0;
			virtual size_t ReadUInt32s(const wchar_t* pszName, size_t count, uint32_t* arr) = 0;
			virtual size_t ReadInt64s(const wchar_t* pszName, size_t count, int64_t* arr) = 0;
			virtual size_t ReadUInt64s(const wchar_t* pszName, size_t count, uint64_t* arr) = 0;
			virtual size_t ReadFloat4s(const wchar_t* pszName, size_t count, float4_t* arr) = 0;
			virtual size_t ReadFloat8s(const wchar_t* pszName, size_t count, float8_t* arr) = 0;
			virtual size_t ReadStrings(const wchar_t* pszName, size_t count, string_t* arr) = 0;
			virtual size_t ReadGuids(const wchar_t* pszName, size_t count, guid_t* arr) = 0;
			virtual void ReadStructStart(const wchar_t* pszName, const wchar_t* pszType) = 0;
			virtual void ReadStructEnd(const wchar_t* pszName) = 0;
			
			virtual void WriteBooleans(const wchar_t* pszName, size_t count, const bool_t* arr) = 0;
			virtual void WriteBytes(const wchar_t* pszName, size_t count, const byte_t* arr) = 0;
			virtual void WriteInt16s(const wchar_t* pszName, size_t count, const int16_t* arr) = 0;
			virtual void WriteUInt16s(const wchar_t* pszName, size_t count, const uint16_t* arr) = 0;
			virtual void WriteInt32s(const wchar_t* pszName, size_t count, const int32_t* arr) = 0;
			virtual void WriteUInt32s(const wchar_t* pszName, size_t count, const uint32_t* arr) = 0;
			virtual void WriteInt64s(const wchar_t* pszName, size_t count, const int64_t* arr) = 0;
			virtual void WriteUInt64s(const wchar_t* pszName, size_t count, const uint64_t* arr) = 0;
			virtual void WriteFloat4s(const wchar_t* pszName, size_t count, const float4_t* arr) = 0;
			virtual void WriteFloat8s(const wchar_t* pszName, size_t count, const float8_t* arr) = 0;
			virtual void WriteStrings(const wchar_t* pszName, size_t count, const string_t* arr) = 0;
			virtual void WriteGuids(const wchar_t* pszName, size_t count, const guid_t* arr) = 0;
			virtual void WriteStructStart(const wchar_t* pszName, const wchar_t* pszType) = 0;
			virtual void WriteStructEnd(const wchar_t* pszName) = 0;
		};
	}

	namespace System
	{
		interface IMarshaller : public IObject
		{
			virtual void MarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseMarshalData(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual Remoting::IMessage* CreateMessage() = 0;
			virtual IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout = 0) = 0;
			virtual TypeInfo::ITypeInfo* GetTypeInfo(const guid_t& iid) = 0;
		};

		interface IStub : public IObject
		{
			virtual void Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut) = 0;
			virtual bool_t SupportsInterface(const guid_t& iid) = 0;
		};

		interface IStubController : public IObject
		{
			virtual void RemoteRelease(uint32_t release_count) = 0;
			virtual bool_t SupportsInterface(const guid_t& iid) = 0;
			virtual void MarshalStub(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut) = 0;
		};

		interface IProxy : public IObject
		{
			virtual void WriteKey(Remoting::IMessage* pMessage) = 0;
			virtual IMarshaller* GetMarshaller() = 0;
			virtual bool_t IsAlive() = 0;
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_SET_GUIDOF(Omega::Remoting, IMessage, "{044E0896-8A60-49e8-9143-5B1F01D4AE4C}")
OMEGA_SET_GUIDOF(Omega::System, IStub, "{0785F8A6-A6BE-4714-A306-D9886128A40E}")
OMEGA_SET_GUIDOF(Omega::System, IStubController, "{B9AD6795-72FA-45a4-9B91-68CE1D5B6283}")
OMEGA_SET_GUIDOF(Omega::System, IProxy, "{0D4BE871-5AD0-497b-A018-EDEA8C17255B}")
OMEGA_SET_GUIDOF(Omega::System, IMarshaller, "{1C288214-61CD-4bb9-B44D-21813DCB0017}")

#define OMEGA_WIRE_DECLARE_WIRE_READWRITE(o_type,fn_type) \
	inline IException_Safe* wire_read(const wchar_t* pszName, IMessage_Safe* pMessage, o_type& val) \
	{ \
		size_t ret = 0; \
		IException_Safe* pSE = pMessage->OMEGA_CONCAT(OMEGA_CONCAT_R(Read,fn_type),_Safe)(&ret,pszName,1,&val); \
		if (!pSE && ret != 1) \
			pSE = return_safe_exception(ISystemException::Create(EIO)); \
		return pSE; \
	} \
	inline IException_Safe* wire_read(const wchar_t* pszName, IMarshaller_Safe*, IMessage_Safe* pMessage, o_type* pVals, const uint64_t& cbMaxSize) \
	{ \
		IException_Safe* pSE = pMessage->ReadStructStart_Safe(pszName,L"$array_type"); \
		if (pSE) return pSE; \
		uint64_t cbSize = 0; \
		size_t ret = 0; \
		pSE = pMessage->ReadUInt64s_Safe(&ret,L"count",1,&cbSize); \
		if (!pSE && ret != 1) \
			pSE = return_safe_exception(ISystemException::Create(EIO)); \
		if (pSE) return pSE; \
		if (cbSize > cbMaxSize) return return_safe_exception(ISystemException::Create(E2BIG)); \
		marshal_info<size_t>::safe_type::type c = static_cast<marshal_info<size_t>::safe_type::type>(cbSize); \
		pSE = pMessage->OMEGA_CONCAT(OMEGA_CONCAT_R(Read,fn_type),_Safe)(&ret,L"data",c,pVals); \
		if (!pSE && ret != c) \
			pSE = return_safe_exception(ISystemException::Create(EIO)); \
		if (pSE) return pSE; \
		return pMessage->ReadStructEnd_Safe(pszName); \
	} \
	inline IException_Safe* wire_write(const wchar_t* pszName, IMessage_Safe* pMessage, o_type val) \
	{ \
		return pMessage->OMEGA_CONCAT(OMEGA_CONCAT_R(Write,fn_type),_Safe)(pszName,1,&val); \
	} \
	inline IException_Safe* wire_write(const wchar_t* pszName, IMarshaller_Safe*, IMessage_Safe* pMessage, const o_type* pVals, const uint64_t& cbSize) \
	{ \
		IException_Safe* pSE = pMessage->WriteStructStart_Safe(pszName,L"$array_type"); \
		if (pSE) return pSE; \
		pSE = pMessage->WriteUInt64s_Safe(L"count",1,&cbSize); \
		if (pSE) return pSE; \
		marshal_info<size_t>::safe_type::type c = static_cast<marshal_info<size_t>::safe_type::type>(cbSize); \
		pSE = pMessage->OMEGA_CONCAT(OMEGA_CONCAT_R(Write,fn_type),_Safe)(pszName,c,pVals); \
		if (pSE) return pSE; \
		return pMessage->WriteStructEnd_Safe(pszName); \
	}

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			OMEGA_DECLARE_FORWARDS(IMessage,Omega::Remoting,IMessage,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IStub,Omega::System,IStub,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IStubController,Omega::System,IStubController,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IProxy,Omega::System,IProxy,Omega,IObject)
			OMEGA_DECLARE_FORWARDS(IMarshaller,Omega::System,IMarshaller,Omega,IObject)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IMessage,

				OMEGA_METHOD(size_t,ReadBooleans,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),bool_t*,arr))
				OMEGA_METHOD(size_t,ReadBytes,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),byte_t*,arr))
				OMEGA_METHOD(size_t,ReadInt16s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),int16_t*,arr))
				OMEGA_METHOD(size_t,ReadUInt16s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),uint16_t*,arr))
				OMEGA_METHOD(size_t,ReadInt32s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),int32_t*,arr))
				OMEGA_METHOD(size_t,ReadUInt32s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),uint32_t*,arr))
				OMEGA_METHOD(size_t,ReadInt64s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),int64_t*,arr))
				OMEGA_METHOD(size_t,ReadUInt64s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),uint64_t*,arr))
				OMEGA_METHOD(size_t,ReadFloat4s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),float4_t*,arr))
				OMEGA_METHOD(size_t,ReadFloat8s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),float8_t*,arr))
				OMEGA_METHOD(size_t,ReadStrings,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),string_t*,arr))
				OMEGA_METHOD(size_t,ReadGuids,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),guid_t*,arr))
				OMEGA_METHOD_VOID(ReadStructStart,2,((in),const wchar_t*,pszName,(in),const wchar_t*,pszType))
				OMEGA_METHOD_VOID(ReadStructEnd,1,((in),const wchar_t*,pszName))

				OMEGA_METHOD_VOID(WriteBooleans,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const bool_t*,arr))
				OMEGA_METHOD_VOID(WriteBytes,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const byte_t*,arr))
				OMEGA_METHOD_VOID(WriteInt16s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const int16_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt16s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const uint16_t*,arr))
				OMEGA_METHOD_VOID(WriteInt32s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const int32_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt32s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const uint32_t*,arr))
				OMEGA_METHOD_VOID(WriteInt64s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const int64_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt64s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const uint64_t*,arr))
				OMEGA_METHOD_VOID(WriteFloat4s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const float4_t*,arr))
				OMEGA_METHOD_VOID(WriteFloat8s,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const float8_t*,arr))
				OMEGA_METHOD_VOID(WriteStrings,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const string_t*,arr))
				OMEGA_METHOD_VOID(WriteGuids,3,((in),const wchar_t*,pszName,(in),size_t,count,(in)(size_is(count)),const guid_t*,arr))
				OMEGA_METHOD_VOID(WriteStructStart,2,((in),const wchar_t*,pszName,(in),const wchar_t*,pszType))
				OMEGA_METHOD_VOID(WriteStructEnd,1,((in),const wchar_t*,pszName))
			)
			typedef IMessage_Impl_Safe<IObject_Safe> IMessage_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IStub,

				OMEGA_METHOD_VOID(Invoke,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
			)
			typedef IStub_Impl_Safe<IObject_Safe> IStub_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IStubController,

				OMEGA_METHOD_VOID(RemoteRelease,1,((in),uint32_t,release_count))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD_VOID(MarshalStub,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
			)
			typedef IStubController_Impl_Safe<IObject_Safe> IStubController_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IProxy,

				OMEGA_METHOD_VOID(WriteKey,1,((in),Remoting::IMessage*,pMessage))
				OMEGA_METHOD(IMarshaller*,GetMarshaller,0,())
				OMEGA_METHOD(bool_t,IsAlive,0,())
			)
			typedef IProxy_Impl_Safe<IObject_Safe> IProxy_Safe;

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IMarshaller,

				OMEGA_METHOD_VOID(MarshalInterface,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD_VOID(UnmarshalInterface,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
				OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),const wchar_t*,pszName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD(Remoting::IMessage*,CreateMessage,0,())
				OMEGA_METHOD(IException*,SendAndReceive,4,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pSend,(out),Remoting::IMessage*&,pRecv,(in),uint32_t,timeout))
				OMEGA_METHOD(TypeInfo::ITypeInfo*,GetTypeInfo,1,((in),const guid_t&,iid))
			)
			typedef IMarshaller_Impl_Safe<IObject_Safe> IMarshaller_Safe;

			OMEGA_WIRE_DECLARE_WIRE_READWRITE(byte_t,Bytes)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(bool_t,Booleans)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int16_t,Int16s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint16_t,UInt16s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int32_t,Int32s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint32_t,UInt32s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int64_t,Int64s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint64_t,UInt64s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(float4_t,Float4s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(float8_t,Float8s)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(guid_t,Guids)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(string_t,Strings)

			/*template <class T, class S>
			inline IException_Safe* wire_read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, T* pVals, S cbMaxSize)
			{
				size_t cbSize = 0;
				IException_Safe* pSE = pMessage->ReadArrayStart_Safe(&cbSize,pszName,marshal_info<T>::type_name());
				if (pSE) 
					return pSE;

				if (cbSize > cbMaxSize) 
					return return_safe_exception(ISystemException::Create(E2BIG));

				if (cbSize && pVals)
				{	 
					for (size_t i=0;i<cbSize;++i)
					{
						pSE = marshal_info<T>::wire_type::read(pszName,pManager,pMessage,pVals[i]);
						if (pSE)
							return pSE;
					}
				}

				return pMessage->ReadArrayEnd_Safe(pszName);
			}*/
			
			/*template <class T, class S>
			inline IException_Safe* wire_write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const T* pVals, const S& cbSize)
			{
				IException_Safe* pSE = wire_write(pMessage,static_cast<const uint64_t&>(cbSize));
				if (pSE)
					return pSE;

				for (uint64_t i=0;i<cbSize;++i)
				{
					pSE = marshal_info<T>::wire_type::write(pManager,pMessage,pVals[i]);
					if (pSE)
						return pSE;
				}

				return 0;
			}*/

			template <class T>
			class std_wire_type
			{
			public:
				typedef typename marshal_info<T>::safe_type::type type;
				typedef typename marshal_info<T>::safe_type::type real_type;

				static IException_Safe* init(type& val)
				{
					val = default_value<type>::value();
					return 0;
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe*, IMessage_Safe* pMessage, type& val)
				{
					return wire_read(pszName,pMessage,val);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe*, IMessage_Safe* pMessage, const type& val)
				{
					return wire_write(pszName,pMessage,val);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe*, IMessage_Safe* pMessage, const type&)
				{
					// Just read the value back, moving the read pointer correctly
					type val = default_value<type>::value();
					return wire_read(pszName,pMessage,val);
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
				typedef typename marshal_info<T>::wire_type::type type;
				typedef typename marshal_info<T>::wire_type::type real_type;

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val)
				{
					return marshal_info<T>::wire_type::read(pszName,pManager,pMessage,const_cast<typename marshal_info<T>::wire_type::type&>(val));
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val)
				{
					return marshal_info<T>::wire_type::write(pszName,pManager,pMessage,val);
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

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::read(pszName,pManager,pMessage,*val);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::read(pszName,pManager,pMessage,*val,piid);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::write(pszName,pManager,pMessage,*val);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::write(pszName,pManager,pMessage,*val,piid);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val)
				{
					return marshal_info<T>::wire_type::unpack(pszName,pManager,pMessage,val.m_val);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T&>::safe_type::type val)
				{
					return marshal_info<T>::wire_type::unpack(pszName,pManager,pMessage,*val);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T&>::safe_type::type val, const guid_t* piid)
				{
					return marshal_info<T>::wire_type::unpack(pszName,pManager,pMessage,*val,piid);
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

					IException_Safe* init(const uint64_t& cbSize)
					{
						try
						{
#if !defined(OMEGA_64)
							if (cbSize > (size_t)-1 / sizeof(typename marshal_info<typename remove_const<T>::type>::wire_type::type))
								OMEGA_THROW(E2BIG);
#endif

							m_alloc_size = cbSize;
							OMEGA_NEW(m_pVals,typename marshal_info<typename remove_const<T>::type>::wire_type::type[(size_t)m_alloc_size]);
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

					uint64_t m_alloc_size;
					typename marshal_info<typename remove_const<T>::type>::wire_type::type* m_pVals;
				};
				typedef array_holder type;

				static IException_Safe* init(type& val, const uint64_t* cbSize)
				{
					return val.init(*cbSize);
				}

				static IException_Safe* init(type& val, const uint32_t* cbSize)
				{
					return val.init(*cbSize);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint64_t* cbSize)
				{
					return wire_read(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return wire_read(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, type& val, const uint64_t& cbSize)
				{
					IException_Safe* pSE = val.init(cbSize);
					if (pSE)
						return pSE;

					return wire_read(pszName,pManager,pMessage,val.m_pVals,cbSize);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, type& val, const uint64_t* cbSize)
				{
					return read(pszName,pManager,pMessage,val,*cbSize);
				}

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, type& val, const uint32_t* cbSize)
				{
					return read(pszName,pManager,pMessage,val,*cbSize);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const typename marshal_info<T>::wire_type::type* pVals, const uint64_t& cbSize)
				{
					return wire_write(pszName,pManager,pMessage,pVals,cbSize);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const typename marshal_info<T>::wire_type::type* pVals, const uint64_t* cbSize)
				{
					return write(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return write(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint64_t& cbSize)
				{
					// Only write back what we have room for...
					if (cbSize > val.m_alloc_size)
						return wire_write(pszName,pManager,pMessage,val.m_pVals,val.m_alloc_size);
					else
						return wire_write(pszName,pManager,pMessage,val.m_pVals,cbSize);					
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint64_t* cbSize)
				{
					return write(pszName,pManager,pMessage,val,*cbSize);
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint32_t* cbSize)
				{
					return write(pszName,pManager,pMessage,val,*cbSize);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint64_t* cbSize)
				{
					return wire_read(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint32_t* cbSize)
				{
					return wire_read(pszName,pManager,pMessage,pVals,*cbSize);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint64_t& cbSize)
				{
					// Only read what we have room for...
					if (cbSize > val.m_alloc_size)
						return wire_read(pszName,pManager,pMessage,val.m_pVals,val.m_alloc_size);
					else
						return wire_read(pszName,pManager,pMessage,val.m_pVals,cbSize);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint64_t* cbSize)
				{
					return unpack(pszName,pManager,pMessage,val,*cbSize);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, const type& val, const uint32_t* cbSize)
				{
					return unpack(pszName,pManager,pMessage,val,*cbSize);
				}

				template <class S>
				static IException_Safe* no_op(bool, S)
				{
					return 0;
				}
			};

			template <>
			class std_wire_type_array<void>
			{
			public:
				typedef void* type;

				static IException_Safe* read(const wchar_t*, IMarshaller_Safe*, IMessage_Safe*, type&)
				{
					return return_safe_exception(ISystemException::Create(EINVAL));
				}

				static IException_Safe* write(const wchar_t*, IMarshaller_Safe*, IMessage_Safe*, const type&)
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

				static IException_Safe* read(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, real_type& pI, const guid_t* piid = 0)
				{
					IObject_Safe* p = 0;
					IException_Safe* pSE = pManager->UnmarshalInterface_Safe(pszName,pMessage,piid ? piid : &OMEGA_GUIDOF(I),&p);
					if (pSE)
						return pSE;
					pI = static_cast<real_type>(p);
					return 0;
				}

				static IException_Safe* write(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, real_type pI, const guid_t* piid = 0)
				{
					return pManager->MarshalInterface_Safe(pszName,pMessage,piid ? piid : &OMEGA_GUIDOF(I),pI);
				}

				static IException_Safe* unpack(const wchar_t* pszName, IMarshaller_Safe* pManager, IMessage_Safe* pMessage, real_type pI, const guid_t* piid = 0)
				{
					return pManager->ReleaseMarshalData_Safe(pszName,pMessage,piid ? piid : &OMEGA_GUIDOF(I),pI);
				}

				static IException_Safe* no_op(bool, const guid_t* = 0)
				{
					return 0;
				}
			};

			typedef IException_Safe* (OMEGA_CALL *pfnCreateProxy)(IProxy_Safe* pProxy, IMarshaller_Safe* pManager, IObject_Safe** ppProxy);
			typedef IException_Safe* (OMEGA_CALL *pfnCreateStub)(IStubController_Safe* pController, IMarshaller_Safe* pManager, IObject_Safe* pObject, IStub_Safe** ppStub);	
			inline void RegisterAutoProxyStubCreators(const guid_t& iid, pfnCreateProxy pfnProxy, pfnCreateStub pfnStub);

			typedef IException_Safe* (OMEGA_CALL *pfnCreateTypeInfo)(ITypeInfo_Safe** ppTypeInfo);
			inline void RegisterAutoTypeInfo(const guid_t& iid, pfnCreateTypeInfo pfnTypeInfo);

			template <class S>
			IStub_Safe* CreateStub(IStubController_Safe* pController, IMarshaller_Safe* pManager, IObject_Safe* pObject)
			{
				S* pS = 0;
				OMEGA_NEW(pS,S(pController,pManager,pObject));
				return pS;
			}

			template <class I>
			class IObject_Stub : public IStub_Safe
			{
			public:
				IObject_Stub(IStubController_Safe* pController, IMarshaller_Safe* pManager, IObject_Safe* pObj) :
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
					if (*piid == OMEGA_GUIDOF(IObject) ||
						*piid == OMEGA_GUIDOF(IStub))
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

				typedef IException_Safe* (*MethodTableEntry)(void* pParam, IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut);

				virtual IException_Safe* OMEGA_CALL Invoke_Safe(IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut)
				{
					// Read the method id
					uint32_t method_id = 0;
					IException_Safe* pSE = wire_read(L"$method_id",pParamsIn,method_id);
					if (pSE)
						return pSE;

					return Internal_Invoke_Safe(method_id,pParamsIn,pParamsOut);
				}

				virtual IException_Safe* Internal_Invoke_Safe(uint32_t method_id, IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut)
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
				static const uint32_t MethodCount = 3; // This must match IObject_Proxy

				IMarshaller_Safe*  m_pManager;
				I*                  m_pS;

			protected:
				virtual ~IObject_Stub()
				{
					m_pS->Release_Safe();
				}

			private:
				Threading::AtomicOp<uint32_t> m_refcount;
				IStubController_Safe*         m_pController;

				IObject_Stub(const IObject_Stub&) {};
				IObject_Stub& operator =(const IObject_Stub&) {};

				static IException_Safe* RemoteRelease_Wire(void* pParam, IMessage_Safe* pParamsIn, IMessage_Safe*)
				{
					uint32_t release_count = 0;
					IException_Safe* pSE = wire_read(L"release_count",pParamsIn,release_count);
					if (pSE)
						return pSE;

					return static_cast<IObject_Stub<I>*>(pParam)->m_pController->RemoteRelease_Safe(release_count);
				}

				static IException_Safe* QueryInterface_Wire(void* pParam, IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut)
				{
					marshal_info<guid_t>::wire_type::type iid;
					IException_Safe* pSE = marshal_info<guid_t>::wire_type::read(L"iid",static_cast<IObject_Stub<I>*>(pParam)->m_pManager,pParamsIn,iid);
					if (pSE)
						return pSE;

					marshal_info<bool_t&>::wire_type::type bQI = false;
					pSE = static_cast<IObject_Stub<I>*>(pParam)->m_pController->SupportsInterface_Safe(bQI,&iid);
					if (pSE)
						return pSE;

					return marshal_info<bool_t&>::wire_type::write(L"bQI",static_cast<IObject_Stub<I>*>(pParam)->m_pManager,pParamsOut,bQI);
				}

				static IException_Safe* MarshalStub_Wire(void* pParam, IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut)
				{
					return static_cast<IObject_Stub<I>*>(pParam)->m_pController->MarshalStub_Safe(pParamsIn,pParamsOut);
				}
			};

			template <class I_Proxy>
			class ProxyImpl : public IObject_Safe
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
					if (*piid == OMEGA_GUIDOF(IObject))
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

				static IObject_Safe* Create(IProxy_Safe* pProxy, IMarshaller_Safe* pManager)
				{
					IObject_Safe* pRet = 0;
					OMEGA_NEW(pRet,ProxyImpl(pProxy,pManager));
					return pRet;
				}

			private:
				Threading::AtomicOp<uint32_t> m_refcount;
				I_Proxy                       m_contained;

				ProxyImpl(IProxy_Safe* pProxy, IMarshaller_Safe* pManager) :
					m_refcount(1), m_contained(pProxy,pManager)
				{ }

				virtual ~ProxyImpl()
				{ }
			};

			template <class Base>
			class IObject_Proxy : public Base
			{
			public:
				IObject_Proxy(IProxy_Safe* pProxy, IMarshaller_Safe* pManager) :
					m_pManager(pManager), m_pProxy(pProxy)
				{
					m_pManager->AddRef_Safe();
				}

				virtual ~IObject_Proxy()
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

				static const uint32_t MethodCount = 3;	// This must match IObject_Stub

			protected:
				IMarshaller_Safe*  m_pManager;

				IException_Safe* CreateMessage(auto_iface_safe_ptr<IMessage_Safe>& pMessage)
				{
					IMessage_Safe* p = 0;
					IException_Safe* pSE = m_pManager->CreateMessage_Safe(&p);
					if (pSE)
						return pSE;
					pMessage.attach(p);
					return 0;
				}

				IException_Safe* SendAndReceive(IException_Safe*& pRet, TypeInfo::MethodAttributes_t attribs, IMessage_Safe* pParamsOut, auto_iface_safe_ptr<IMessage_Safe>& pParamsIn, uint32_t timeout = 0)
				{
					IMessage_Safe* p = 0;
					IException_Safe* pSE = m_pManager->SendAndReceive_Safe(&pRet,attribs,pParamsOut,&p,timeout);
					if (pSE)
						return pSE;
					pParamsIn.attach(p);
					return 0;
				}

				IException_Safe* WriteKey(IMessage_Safe* pMessage, const guid_t& iid)
				{
					IException_Safe* pSE = m_pProxy->WriteKey_Safe(pMessage);
					if (pSE)
						return pSE;

					return wire_write(L"$iid",pMessage,iid);
				}

			private:
				IProxy_Safe* m_pProxy;
			};

			OMEGA_QI_MAGIC(Omega::Remoting,IMessage)

			OMEGA_QI_MAGIC(Omega::System,IStub)
			OMEGA_QI_MAGIC(Omega::System,IStubController)
			OMEGA_QI_MAGIC(Omega::System,IProxy)
			OMEGA_QI_MAGIC(Omega::System,IMarshaller)

			OMEGA_WIRE_MAGIC(Omega,IObject)

			// These are the remoteable interfaces
			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega, IException,

				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
				OMEGA_METHOD(string_t,GetSource,0,())
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
			(
				Omega::TypeInfo, ITypeInfo,

				OMEGA_METHOD(string_t,GetName,0,())
				OMEGA_METHOD(guid_t,GetIID,0,())
				OMEGA_METHOD(Omega::TypeInfo::ITypeInfo*,GetBaseType,0,())
				OMEGA_METHOD(uint32_t,GetMethodCount,0,())
				OMEGA_METHOD_VOID(GetMethodInfo,6,((in),uint32_t,method_idx,(out),string_t&,strName,(out),TypeInfo::MethodAttributes_t&,attribs,(out),uint32_t&,timeout,(out),byte_t&,param_count,(out),TypeInfo::Types_t&,return_type))
				OMEGA_METHOD_VOID(GetParamInfo,5,((in),uint32_t,method_idx,(in),byte_t,param_idx,(out),string_t&,strName,(out),TypeInfo::Types_t&,type,(out),TypeInfo::ParamAttributes_t&,attribs))
				OMEGA_METHOD(byte_t,GetAttributeRef,3,((in),uint32_t,method_idx,(in),byte_t,param_idx,(in),TypeInfo::ParamAttributes_t,attrib))
			)
		}
	}
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_RegisterAutoProxyStubCreators,3,((in),const Omega::guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub));
void Omega::System::MetaInfo::RegisterAutoProxyStubCreators(const guid_t& iid, pfnCreateProxy pfnProxy, pfnCreateStub pfnStub)
{
	Omega_RegisterAutoProxyStubCreators(iid,pfnProxy,pfnStub);
}

OMEGA_EXPORTED_FUNCTION_VOID(Omega_RegisterAutoTypeInfo,2,((in),const Omega::guid_t&,iid,(in),void*,pfnTypeInfo));
void Omega::System::MetaInfo::RegisterAutoTypeInfo(const guid_t& iid, pfnCreateTypeInfo pfnTypeInfo)
{
	Omega_RegisterAutoTypeInfo(iid,pfnTypeInfo);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_H_INCLUDED_
