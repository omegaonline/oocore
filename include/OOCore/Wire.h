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
			virtual bool_t RemoteQueryInterface(const guid_t& iid) = 0;
			virtual void MarshalStub(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut) = 0;
		};

		interface IProxy : public IObject
		{
			//virtual void Pin() = 0;
			//virtual void Unpin() = 0;
			//virtual bool_t SupportsInterface(const guid_t& iid) = 0;
			virtual void WriteKey(Remoting::IMessage* pMessage) = 0;
			virtual void UnpackKey(Remoting::IMessage* pMessage) = 0;
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
	inline void wire_read(const wchar_t* pszName, Remoting::IMessage* pMessage, o_type& val) \
	{ \
		size_t ret = pMessage->OMEGA_CONCAT_R(Read,fn_type)(pszName,1,&val); \
		if (ret != 1) \
			OMEGA_THROW(L"Failed to read from IMessage"); \
	} \
	inline void wire_read(const wchar_t* pszName, IMarshaller*, Remoting::IMessage* pMessage, o_type* pVals, const uint64_t& cbMaxSize) \
	{ \
		pMessage->ReadStructStart(pszName,L"$array_type"); \
		uint64_t cbSize = 0; \
		size_t ret = pMessage->ReadUInt64s(L"count",1,&cbSize); \
		if (ret != 1) \
			OMEGA_THROW(L"Failed to read from IMessage"); \
		if (cbSize > cbMaxSize || cbSize > (size_t)-1) \
			OMEGA_THROW(L"Array too big"); \
		size_t c = static_cast<size_t>(cbSize); \
		ret = pMessage->OMEGA_CONCAT_R(Read,fn_type)(L"data",c,pVals); \
		if (ret != c) \
			OMEGA_THROW(L"Failed to read from IMessage"); \
		pMessage->ReadStructEnd(pszName); \
	} \
	inline void wire_write(const wchar_t* pszName, Remoting::IMessage* pMessage, o_type val) \
	{ \
		pMessage->OMEGA_CONCAT_R(Write,fn_type)(pszName,1,&val); \
	} \
	inline void wire_write(const wchar_t* pszName, IMarshaller*, Remoting::IMessage* pMessage, const o_type* pVals, const uint64_t& cbSize) \
	{ \
		pMessage->WriteStructStart(pszName,L"$array_type"); \
		pMessage->WriteUInt64s(L"count",1,&cbSize); \
		pMessage->OMEGA_CONCAT_R(Write,fn_type)(pszName,static_cast<size_t>(cbSize),pVals); \
		pMessage->WriteStructEnd(pszName); \
	}

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IMessage)
			OMEGA_DECLARE_FORWARDS(Omega::System,IStub)
			OMEGA_DECLARE_FORWARDS(Omega::System,IStubController)
			OMEGA_DECLARE_FORWARDS(Omega::System,IProxy)
			OMEGA_DECLARE_FORWARDS(Omega::System,IMarshaller)

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
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IStub,

				OMEGA_METHOD_VOID(Invoke,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
			)
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IStubController,

				OMEGA_METHOD_VOID(RemoteRelease,1,((in),uint32_t,release_count))
				OMEGA_METHOD(bool_t,RemoteQueryInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD_VOID(MarshalStub,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
			)
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::System, IProxy,

				//OMEGA_METHOD_VOID(Pin,0,())
				//OMEGA_METHOD_VOID(Unpin,0,())
				//OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD_VOID(WriteKey,1,((in),Remoting::IMessage*,pMessage))
				OMEGA_METHOD_VOID(UnpackKey,1,((in),Remoting::IMessage*,pMessage))
				OMEGA_METHOD(IMarshaller*,GetMarshaller,0,())
				OMEGA_METHOD(bool_t,IsAlive,0,())
			)
			
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
			inline IException_Safe* wire_read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, T* pVals, S cbMaxSize)
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
			inline IException_Safe* wire_write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const T* pVals, const S& cbSize)
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

				static void init(type& val)
				{
					val = default_value<type>::value();
					return 0;
				}

				static void read(const wchar_t* pszName, IMarshaller*, Remoting::IMessage* pMessage, real_type& val)
				{
					wire_read(pszName,pMessage,val);
				}

				static void write(const wchar_t* pszName, IMarshaller*, Remoting::IMessage* pMessage, const real_type& val)
				{
					wire_write(pszName,pMessage,val);
				}

				static void unpack(const wchar_t* pszName, IMarshaller*, Remoting::IMessage* pMessage, const real_type&)
				{
					// Just read the value back, moving the read pointer correctly
					type val = default_value<type>::value();
					wire_read(pszName,pMessage,val);
				}

				static void no_op(bool)
				{ }
			};

			template <class T>
			class std_wire_type<const T>
			{
			public:
				typedef typename marshal_info<T>::wire_type::type type;
				typedef typename marshal_info<T>::wire_type::type real_type;

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const real_type& val)
				{
					marshal_info<T>::wire_type::read(pszName,pManager,pMessage,const_cast<typename marshal_info<T>::wire_type::real_type&>(val));
				}

				static void write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const real_type& val)
				{
					marshal_info<T>::wire_type::write(pszName,pManager,pMessage,val);
				}

				static void no_op(bool)
				{ }
			};

			template <class T>
			class std_wire_type<T&>
			{
			public:
				class ref_holder
				{
				public:
					ref_holder(const typename marshal_info<T>::wire_type::type& val = default_value<typename marshal_info<T>::wire_type::type>::value()) : m_val(val)
					{}

					operator typename marshal_info<T>::wire_type::real_type&()
					{
						return m_val;
					}

					typename marshal_info<T>::wire_type::type m_val;
				};
				typedef ref_holder type;

				static void init(ref_holder&)
				{ }

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, typename marshal_info<T>::wire_type::real_type& val)
				{
					marshal_info<T>::wire_type::read(pszName,pManager,pMessage,val);
				}

				static void write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const typename marshal_info<T>::wire_type::real_type& val)
				{
					marshal_info<T>::wire_type::write(pszName,pManager,pMessage,val);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const typename marshal_info<T>::wire_type::real_type& val)
				{
					marshal_info<T>::wire_type::unpack(pszName,pManager,pMessage,val);
				}

				static void no_op(bool)
				{ }
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

					void init(const uint64_t& cbSize)
					{
					#if !defined(OMEGA_64)
						if (cbSize > (size_t)-1 / sizeof(typename marshal_info<typename remove_const<T>::type>::wire_type::type))
							OMEGA_THROW(L"Attempt to marshal too many array items");
					#endif

						m_alloc_size = cbSize;
						OMEGA_NEW(m_pVals,typename marshal_info<typename remove_const<T>::type>::wire_type::type[(size_t)m_alloc_size]);
					}

					operator typename marshal_info<T>::wire_type::type*()
					{
						return m_pVals;
					}

					uint64_t m_alloc_size;
					typename marshal_info<typename remove_const<T>::type>::wire_type::type* m_pVals;
				};
				typedef array_holder type;

				static void init(type& val, const uint64_t& cbSize)
				{
					val.init(cbSize);
				}

				static void init(type& val, uint32_t cbSize)
				{
					val.init(cbSize);
				}

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint64_t& cbSize)
				{
					wire_read(pszName,pManager,pMessage,pVals,cbSize);
				}

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					wire_read(pszName,pManager,pMessage,pVals,cbSize);
				}

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, type& val, const uint64_t& cbSize)
				{
					val.init(cbSize);
					wire_read(pszName,pManager,pMessage,val.m_pVals,cbSize);
				}

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, type& val, uint32_t cbSize)
				{
					read(pszName,pManager,pMessage,val,static_cast<uint64_t>(cbSize));
				}

				static void write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const typename marshal_info<T>::wire_type::type* pVals, const uint64_t& cbSize)
				{
					wire_write(pszName,pManager,pMessage,pVals,cbSize);
				}

				static void write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, const uint64_t& cbSize)
				{
					// Only write back what we have room for...
					if (cbSize > val.m_alloc_size)
						wire_write(pszName,pManager,pMessage,val.m_pVals,val.m_alloc_size);
					else
						wire_write(pszName,pManager,pMessage,val.m_pVals,cbSize);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, typename marshal_info<T>::wire_type::type* pVals, const uint64_t& cbSize)
				{
					wire_read(pszName,pManager,pMessage,pVals,cbSize);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, typename marshal_info<T>::wire_type::type* pVals, uint32_t cbSize)
				{
					wire_read(pszName,pManager,pMessage,pVals,cbSize);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, const uint64_t& cbSize)
				{
					// Only read what we have room for...
					if (cbSize > val.m_alloc_size)
						wire_read(pszName,pManager,pMessage,val.m_pVals,val.m_alloc_size);
					else
						wire_read(pszName,pManager,pMessage,val.m_pVals,cbSize);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, uint32_t cbSize)
				{
					return unpack(pszName,pManager,pMessage,val,static_cast<uint64_t>(cbSize));
				}				

				template <class S>
				static void no_op(bool, S)
				{ }
			};

			template <>
			class std_wire_type_array<void>
			{
			public:
				typedef void* type;

				static void read(const wchar_t*, IMarshaller*, Remoting::IMessage*, type&)
				{
					OMEGA_THROW(L"Cannot marshal void*");
				}

				static void write(const wchar_t*, IMarshaller*, Remoting::IMessage*, const type&)
				{
					OMEGA_THROW(L"Cannot marshal void*");
				}
			};

			template <class I>
			class iface_wire_type
			{
			public:
				typedef I* real_type;

				class if_holder
				{
				public:
					if_holder(I* val = 0) : m_val(val)
					{
						if (m_val)
							m_val->AddRef();
					}

					~if_holder()
					{
						if (m_val)
							m_val->Release();
					}

					operator I*&()
					{
						return m_val;
					}

					I* m_val;
				};
				typedef if_holder type;

				static void init(type&)
				{ }

				static void read(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, I*& pI)
				{
					IObject* p = 0;
					pManager->UnmarshalInterface(pszName,pMessage,OMEGA_GUIDOF(I),p);
					pI = static_cast<I*>(p);
				}

				static void write(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, I* pI)
				{
					pManager->MarshalInterface(pszName,pMessage,OMEGA_GUIDOF(I),pI);
				}

				static void unpack(const wchar_t* pszName, IMarshaller* pManager, Remoting::IMessage* pMessage, I* pI)
				{
					pManager->ReleaseMarshalData(pszName,pMessage,OMEGA_GUIDOF(I),pI);
				}

				static void no_op(bool)
				{ }
			};

			typedef SafeShim* (OMEGA_CALL *pfnCreateWireProxy)(SafeShim* proxy_shim, SafeShim** wire_proxy);
			typedef SafeShim* (OMEGA_CALL *pfnCreateWireStub)(SafeShim* pController, SafeShim* pManager, SafeShim* pObject, SafeShim** ppStub);

			inline void RegisterAutoProxyStubCreators(const guid_t& iid, pfnCreateWireProxy pfnProxy, pfnCreateWireStub pfnStub);
			inline void UnregisterAutoProxyStubCreators(const guid_t& iid, pfnCreateWireProxy pfnProxy, pfnCreateWireStub pfnStub);

			template <class I>
			class Wire_Proxy;

			template <class I>
			inline SafeShim* create_wire_proxy(SafeShim* proxy_shim, SafeShim** ret)
			{
				try
				{
					auto_iface_ptr<IProxy> ptrProxy = static_cast<IProxy*>(create_proxy(proxy_shim));
					*ret = Wire_Proxy<I>::create(ptrProxy);
					return 0;
				}
				catch (IException* pE)
				{
					return return_safe_exception(pE);
				}
			}

			template <>
			class Wire_Proxy<IObject>
			{
			public:
				static SafeShim* create(IProxy* pProxy)
				{
					Wire_Proxy* pThis;
					OMEGA_NEW(pThis,Wire_Proxy(pProxy,&OMEGA_GUIDOF(IObject)));
					return &pThis->m_shim;
				}

			protected:
				Wire_Proxy(IProxy* pProxy, const guid_t* iid) : 
					 m_ptrMarshaller(pProxy->GetMarshaller()),
					 m_ptrProxy(pProxy)					 
				{
					m_ptrProxy->AddRef();
					m_refcount.AddRef(); 

					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = iid;
				}

				virtual ~Wire_Proxy()
				{}

				auto_iface_ptr<Remoting::IMessage> CreateMessage(uint32_t method_id)
				{
					auto_iface_ptr<Remoting::IMessage> ptrMessage = m_ptrMarshaller->CreateMessage();
					bool unpack = false;
					try
					{
						ptrMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");
						unpack = true;
						m_ptrProxy->WriteKey(ptrMessage);
						wire_write(L"$iid",ptrMessage,*m_shim.m_iid);
						wire_write(L"$method_id",ptrMessage,method_id);
						return ptrMessage;
					}
					catch (...)
					{
						if (unpack)
						{
							ptrMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
							m_ptrProxy->UnpackKey(ptrMessage);
						}
						throw;
					}
				}

				void UnpackHeader(Remoting::IMessage* pMessage)
				{
					pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");
					m_ptrProxy->UnpackKey(pMessage);
					uint32_t key1; guid_t key2;
					wire_read(L"$iid",pMessage,key2);
					wire_read(L"$method_id",pMessage,key1);
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						0, 0 // Pin and Unpin are not called...
					};
					return &vt;
				}

				static const uint32_t MethodCount = 3;	// This must match the stub

				auto_iface_ptr<IMarshaller> m_ptrMarshaller;
				SafeShim                    m_shim;
								
			private:
				Threading::AtomicRefCount   m_refcount;
				auto_iface_ptr<IProxy>      m_ptrProxy;
								
				Wire_Proxy(const Wire_Proxy&) {}
				Wire_Proxy& operator = (const Wire_Proxy&) { return *this; }
												
				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					if (m_refcount.Release())
						delete this;
				}

				SafeShim* QueryInterface(const guid_t& iid)
				{
					if (IsDerived(iid))
					{
						AddRef();
						return &m_shim;
					}

					return 0;
				}

				static SafeShim* OMEGA_CALL AddRef_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
					try
					{
						static_cast<Wire_Proxy*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL Release_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
					try
					{
						static_cast<Wire_Proxy*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL QueryInterface_Safe(SafeShim* shim, SafeShim** retval, const guid_t* iid)
				{
					SafeShim* except = 0;
					try
					{
						*retval = static_cast<Wire_Proxy*>(shim->m_stub)->QueryInterface(*iid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			template <class I>
			class Wire_Stub;

			template <class I>
			inline SafeShim* create_wire_stub(SafeShim* shim_Controller, SafeShim* shim_Marshaller, SafeShim* shim_I, SafeShim** ret)
			{
				try
				{
					auto_iface_ptr<IStubController> ptrController = static_cast<IStubController*>(create_proxy(shim_Controller));
					auto_iface_ptr<IMarshaller> ptrMarshaller = static_cast<IMarshaller*>(create_proxy(shim_Marshaller));
					auto_iface_ptr<IObject> ptrI = create_proxy(shim_I);
					
					*ret = Wire_Stub<I>::create(ptrController,ptrMarshaller,ptrI);
					return 0;
				}
				catch (IException* pE)
				{
					return return_safe_exception(pE);
				}
			}

			template <>
			class Wire_Stub<IObject>
			{
			public:
				static SafeShim* create(IStubController* pController, IMarshaller* pMarshaller, IObject* pI)
				{
					Wire_Stub* pThis;
					OMEGA_NEW(pThis,Wire_Stub(pController,pMarshaller,pI));
					return &pThis->m_shim;
				}
				
			protected:
				Wire_Stub(IStubController* pController, IMarshaller* pMarshaller, IObject* pI) :
					m_ptrMarshaller(pMarshaller), m_ptrI(pI), m_pController(pController)
				{
					m_ptrMarshaller->AddRef();
					m_ptrI->AddRef();
		
					m_refcount.AddRef();

					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = &OMEGA_GUIDOF(IStub);
				}

				virtual ~Wire_Stub()
				{}

				virtual bool_t SupportsInterface(const guid_t& iid)
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				typedef void (*MethodTableEntry)(void* pParam, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut);

				virtual void Internal_Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					static const MethodTableEntry MethodTable[] =
					{
						&RemoteRelease_Wire,
						&QueryInterface_Wire,
						&MarshalStub_Wire
					};

					if (method_id < MethodCount)
						return MethodTable[method_id](this,pParamsIn,pParamsOut);
					
					OMEGA_THROW(L"Invoke called with invalid method index");
				}

				template <class I>
				I* get_iface()
				{
					return static_cast<I*>(static_cast<IObject*>(m_ptrI));
				}

				static const uint32_t MethodCount = 3;	// This must match the proxy

				auto_iface_ptr<IMarshaller> m_ptrMarshaller;
				SafeShim                    m_shim;
				
			private:
				auto_iface_ptr<IObject>   m_ptrI;
				IStubController*          m_pController;
				Threading::AtomicRefCount m_refcount;
				
				Wire_Stub(const Wire_Stub&) {}
				Wire_Stub& operator = (const Wire_Stub&) { return *this; }

				static const vtable_info<IStub>::type* get_vt()
				{
					static const vtable_info<IStub>::type vt = 
					{
						{
							&AddRef_Safe,
							&Release_Safe,
							&QueryInterface_Safe,
							0, 0 // Pin and Unpin are not called...
						},
						&Invoke_Safe,
						&SupportsInterface_Safe,
					};
					return &vt;
				}
												
				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					if (m_refcount.Release())
						delete this;
				}

				void Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					// Read the method id
					uint32_t method_id = 0;
					wire_read(L"$method_id",pParamsIn,method_id);

					Internal_Invoke(method_id,pParamsIn,pParamsOut);
				}

				static SafeShim* OMEGA_CALL AddRef_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
					try
					{
						static_cast<Wire_Stub*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL Release_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
					try
					{
						static_cast<Wire_Stub*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL QueryInterface_Safe(SafeShim* shim, SafeShim** retval, const guid_t* iid)
				{
					SafeShim* except = 0;
					try
					{
						if (*iid == OMEGA_GUIDOF(IObject) ||
							*iid == OMEGA_GUIDOF(IStub))
						{
							static_cast<Wire_Stub*>(shim->m_stub)->AddRef();
							*retval = shim;
						}
						else
							*retval = 0;
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL Invoke_Safe(SafeShim* shim, SafeShim* shim_ParamsIn, SafeShim* shim_ParamsOut)
				{
					SafeShim* except = 0;
					try
					{
						auto_iface_ptr<Remoting::IMessage> ptrParamsIn = static_cast<Remoting::IMessage*>(create_proxy(shim_ParamsIn));
						auto_iface_ptr<Remoting::IMessage> ptrParamsOut = static_cast<Remoting::IMessage*>(create_proxy(shim_ParamsOut));
						
						static_cast<Wire_Stub*>(shim->m_stub)->Invoke(ptrParamsIn,ptrParamsOut);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static SafeShim* OMEGA_CALL SupportsInterface_Safe(SafeShim* shim, bool_t* retval, const guid_t* piid)
				{
					SafeShim* except = 0;
					try
					{
						*retval = static_cast<Wire_Stub*>(shim->m_stub)->SupportsInterface(*piid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
				
				static void RemoteRelease_Wire(void* pParam, Remoting::IMessage* pParamsIn, Remoting::IMessage*)
				{
					uint32_t release_count = 0;
					wire_read(L"release_count",pParamsIn,release_count);
					static_cast<Wire_Stub*>(pParam)->m_pController->RemoteRelease(release_count);
				}

				static void QueryInterface_Wire(void* pParam, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					guid_t iid;
					wire_read(L"iid",pParamsIn,iid);
					
					bool_t bQI = static_cast<Wire_Stub*>(pParam)->m_pController->RemoteQueryInterface(iid);
					wire_write(L"bQI",pParamsOut,bQI);
				}

				static void MarshalStub_Wire(void* pParam, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					static_cast<Wire_Stub*>(pParam)->m_pController->MarshalStub(pParamsIn,pParamsOut);
				}
			};
			
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
				OMEGA_METHOD(guid_t,GetParamIid,2,((in),uint32_t,method_idx,(in),byte_t,param_idx))
			)

			OMEGA_QI_MAGIC(Omega::Remoting,IMessage)
			OMEGA_QI_MAGIC(Omega::System,IStub)
			OMEGA_QI_MAGIC(Omega::System,IStubController)
			OMEGA_QI_MAGIC(Omega::System,IProxy)
			OMEGA_QI_MAGIC(Omega::System,IMarshaller)
		}
	}
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_RegisterAutoProxyStubCreators,3,((in),const Omega::guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub));
void Omega::System::MetaInfo::RegisterAutoProxyStubCreators(const guid_t& iid, pfnCreateWireProxy pfnProxy, pfnCreateWireStub pfnStub)
{
	OOCore_RegisterAutoProxyStubCreators(iid,(void*)(pfnProxy),(void*)(pfnStub));
}

OMEGA_EXPORTED_FUNCTION_VOID(OOCore_UnregisterAutoProxyStubCreators,3,((in),const Omega::guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub));
void Omega::System::MetaInfo::UnregisterAutoProxyStubCreators(const guid_t& iid, pfnCreateWireProxy pfnProxy, pfnCreateWireStub pfnStub)
{
	OOCore_UnregisterAutoProxyStubCreators(iid,(void*)(pfnProxy),(void*)(pfnStub));
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_H_INCLUDED_
