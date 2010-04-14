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
			virtual bool_t ReadBoolean(const string_t& strName) = 0;
			virtual byte_t ReadByte(const string_t& strName) = 0;
			virtual int16_t ReadInt16(const string_t& strName) = 0;
			virtual uint16_t ReadUInt16(const string_t& strName) = 0;
			virtual int32_t ReadInt32(const string_t& strName) = 0;
			virtual uint32_t ReadUInt32(const string_t& strName) = 0;
			virtual int64_t ReadInt64(const string_t& strName) = 0;
			virtual uint64_t ReadUInt64(const string_t& strName) = 0;
			virtual float4_t ReadFloat4(const string_t& strName) = 0;
			virtual float8_t ReadFloat8(const string_t& strName) = 0;
			virtual string_t ReadString(const string_t& strName) = 0;
			virtual guid_t ReadGuid(const string_t& strName) = 0;

			virtual void ReadBooleans(const string_t& strName, uint32_t count, bool_t* arr) = 0;
			virtual void ReadBytes(const string_t& strName, uint32_t count, byte_t* arr) = 0;
			virtual void ReadInt16s(const string_t& strName, uint32_t count, int16_t* arr) = 0;
			virtual void ReadUInt16s(const string_t& strName, uint32_t count, uint16_t* arr) = 0;
			virtual void ReadInt32s(const string_t& strName, uint32_t count, int32_t* arr) = 0;
			virtual void ReadUInt32s(const string_t& strName, uint32_t count, uint32_t* arr) = 0;
			virtual void ReadInt64s(const string_t& strName, uint32_t count, int64_t* arr) = 0;
			virtual void ReadUInt64s(const string_t& strName, uint32_t count, uint64_t* arr) = 0;
			virtual void ReadFloat4s(const string_t& strName, uint32_t count, float4_t* arr) = 0;
			virtual void ReadFloat8s(const string_t& strName, uint32_t count, float8_t* arr) = 0;
			virtual void ReadStrings(const string_t& strName, uint32_t count, string_t* arr) = 0;
			virtual void ReadGuids(const string_t& strName, uint32_t count, guid_t* arr) = 0;

			virtual void ReadStructStart(const string_t& strName, const string_t& strType) = 0;
			virtual void ReadStructEnd(const string_t& strName) = 0;

			virtual void WriteBoolean(const string_t& strName, bool_t val) = 0;
			virtual void WriteByte(const string_t& strName, byte_t val) = 0;
			virtual void WriteInt16(const string_t& strName, int16_t val) = 0;
			virtual void WriteUInt16(const string_t& strName, uint16_t val) = 0;
			virtual void WriteInt32(const string_t& strName, int32_t val) = 0;
			virtual void WriteUInt32(const string_t& strName, uint32_t val) = 0;
			virtual void WriteInt64(const string_t& strName, const int64_t& val) = 0;
			virtual void WriteUInt64(const string_t& strName, const uint64_t& val) = 0;
			virtual void WriteFloat4(const string_t& strName, float4_t val) = 0;
			virtual void WriteFloat8(const string_t& strName, const float8_t& val) = 0;
			virtual void WriteString(const string_t& strName, const string_t& val) = 0;
			virtual void WriteGuid(const string_t& strName, const guid_t& val) = 0;

			virtual void WriteBooleans(const string_t& strName, uint32_t count, const bool_t* arr) = 0;
			virtual void WriteBytes(const string_t& strName, uint32_t count, const byte_t* arr) = 0;
			virtual void WriteInt16s(const string_t& strName, uint32_t count, const int16_t* arr) = 0;
			virtual void WriteUInt16s(const string_t& strName, uint32_t count, const uint16_t* arr) = 0;
			virtual void WriteInt32s(const string_t& strName, uint32_t count, const int32_t* arr) = 0;
			virtual void WriteUInt32s(const string_t& strName, uint32_t count, const uint32_t* arr) = 0;
			virtual void WriteInt64s(const string_t& strName, uint32_t count, const int64_t* arr) = 0;
			virtual void WriteUInt64s(const string_t& strName, uint32_t count, const uint64_t* arr) = 0;
			virtual void WriteFloat4s(const string_t& strName, uint32_t count, const float4_t* arr) = 0;
			virtual void WriteFloat8s(const string_t& strName, uint32_t count, const float8_t* arr) = 0;
			virtual void WriteStrings(const string_t& strName, uint32_t count, const string_t* arr) = 0;
			virtual void WriteGuids(const string_t& strName, uint32_t count, const guid_t* arr) = 0;
			
			virtual void WriteStructStart(const string_t& strName, const string_t& strType) = 0;
			virtual void WriteStructEnd(const string_t& strName) = 0;
		};

		inline IMessage* CreateMemoryMessage();
	
		interface IMarshaller : public IObject
		{
			virtual void MarshalInterface(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseMarshalData(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual IMessage* CreateMessage() = 0;
			virtual IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, IMessage* pSend, IMessage*& pRecv, uint32_t timeout = 0) = 0;
		};

		interface IStub : public IObject
		{
			virtual void Invoke(IMessage* pParamsIn, IMessage* pParamsOut) = 0;
			virtual bool_t SupportsInterface(const guid_t& iid) = 0;
		};

		interface IStubController : public IObject
		{
			virtual void RemoteRelease() = 0;
			virtual bool_t RemoteQueryInterface(const guid_t& iid) = 0;
			virtual void MarshalStub(IMessage* pParamsIn, IMessage* pParamsOut) = 0;
		};

		interface IProxy : public IObject
		{
			virtual void WriteKey(IMessage* pMessage) = 0;
			virtual void UnpackKey(IMessage* pMessage) = 0;
			virtual IMarshaller* GetMarshaller() = 0;
			virtual bool_t IsAlive() = 0;
			virtual bool_t RemoteQueryInterface(const guid_t& iid) = 0;
			virtual IObject* QueryIObject() = 0;
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_SET_GUIDOF(Omega::Remoting, IMessage, "{044E0896-8A60-49e8-9143-5B1F01D4AE4C}")
OMEGA_SET_GUIDOF(Omega::Remoting, IStub, "{0785F8A6-A6BE-4714-A306-D9886128A40E}")
OMEGA_SET_GUIDOF(Omega::Remoting, IStubController, "{B9AD6795-72FA-45a4-9B91-68CE1D5B6283}")
OMEGA_SET_GUIDOF(Omega::Remoting, IProxy, "{0D4BE871-5AD0-497b-A018-EDEA8C17255B}")
OMEGA_SET_GUIDOF(Omega::Remoting, IMarshaller, "{1C288214-61CD-4bb9-B44D-21813DCB0017}")

#define OMEGA_WIRE_DECLARE_WIRE_READWRITE(T_type,fn_type) \
	inline void wire_read(const string_t& strName, Remoting::IMessage* pMessage, T_type& val) \
	{ \
		val = pMessage->OMEGA_CONCAT(Read,fn_type)(strName); \
	} \
	inline void wire_read(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, T_type* pVals, uint32_t count) \
	{ \
		pMessage->OMEGA_CONCAT(Read,OMEGA_CONCAT_R(fn_type,s))(strName,count,pVals); \
	} \
	inline void wire_write(const string_t& strName, Remoting::IMessage* pMessage, optimal_param<T_type>::type val) \
	{ \
		pMessage->OMEGA_CONCAT(Write,fn_type)(strName,val); \
	} \
	inline void wire_write(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, const T_type* pVals, uint32_t count) \
	{ \
		pMessage->OMEGA_CONCAT(Write,OMEGA_CONCAT_R(fn_type,s))(strName,count,pVals); \
	}

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IMessage)
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IStub)
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IStubController)
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IProxy)
			OMEGA_DECLARE_FORWARDS(Omega::Remoting,IMarshaller)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IMessage,

				OMEGA_METHOD(bool_t,ReadBoolean,1,((in),const string_t&,strName))
				OMEGA_METHOD(byte_t,ReadByte,1,((in),const string_t&,strName))
				OMEGA_METHOD(int16_t,ReadInt16,1,((in),const string_t&,strName))
				OMEGA_METHOD(uint16_t,ReadUInt16,1,((in),const string_t&,strName))
				OMEGA_METHOD(int32_t,ReadInt32,1,((in),const string_t&,strName))
				OMEGA_METHOD(uint32_t,ReadUInt32,1,((in),const string_t&,strName))
				OMEGA_METHOD(int64_t,ReadInt64,1,((in),const string_t&,strName))
				OMEGA_METHOD(uint64_t,ReadUInt64,1,((in),const string_t&,strName))
				OMEGA_METHOD(float4_t,ReadFloat4,1,((in),const string_t&,strName))
				OMEGA_METHOD(float8_t,ReadFloat8,1,((in),const string_t&,strName))
				OMEGA_METHOD(string_t,ReadString,1,((in),const string_t&,strName))
				OMEGA_METHOD(guid_t,ReadGuid,1,((in),const string_t&,strName))

				OMEGA_METHOD_VOID(ReadBooleans,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),bool_t*,arr))
				OMEGA_METHOD_VOID(ReadBytes,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),byte_t*,arr))
				OMEGA_METHOD_VOID(ReadInt16s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),int16_t*,arr))
				OMEGA_METHOD_VOID(ReadUInt16s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),uint16_t*,arr))
				OMEGA_METHOD_VOID(ReadInt32s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),int32_t*,arr))
				OMEGA_METHOD_VOID(ReadUInt32s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),uint32_t*,arr))
				OMEGA_METHOD_VOID(ReadInt64s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),int64_t*,arr))
				OMEGA_METHOD_VOID(ReadUInt64s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),uint64_t*,arr))
				OMEGA_METHOD_VOID(ReadFloat4s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),float4_t*,arr))
				OMEGA_METHOD_VOID(ReadFloat8s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),float8_t*,arr))
				OMEGA_METHOD_VOID(ReadStrings,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),string_t*,arr))
				OMEGA_METHOD_VOID(ReadGuids,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),guid_t*,arr))

				OMEGA_METHOD_VOID(ReadStructStart,2,((in),const string_t&,strName,(in),const string_t&,strType))
				OMEGA_METHOD_VOID(ReadStructEnd,1,((in),const string_t&,strName))

				OMEGA_METHOD_VOID(WriteBoolean,2,((in),const string_t&,strName,(in),bool_t,val))
				OMEGA_METHOD_VOID(WriteByte,2,((in),const string_t&,strName,(in),byte_t,val))
				OMEGA_METHOD_VOID(WriteInt16,2,((in),const string_t&,strName,(in),int16_t,val))
				OMEGA_METHOD_VOID(WriteUInt16,2,((in),const string_t&,strName,(in),uint16_t,val))
				OMEGA_METHOD_VOID(WriteInt32,2,((in),const string_t&,strName,(in),int32_t,val))
				OMEGA_METHOD_VOID(WriteUInt32,2,((in),const string_t&,strName,(in),uint32_t,val))
				OMEGA_METHOD_VOID(WriteInt64,2,((in),const string_t&,strName,(in),const int64_t&,val))
				OMEGA_METHOD_VOID(WriteUInt64,2,((in),const string_t&,strName,(in),const uint64_t&,val))
				OMEGA_METHOD_VOID(WriteFloat4,2,((in),const string_t&,strName,(in),float4_t,val))
				OMEGA_METHOD_VOID(WriteFloat8,2,((in),const string_t&,strName,(in),const float8_t&,val))
				OMEGA_METHOD_VOID(WriteString,2,((in),const string_t&,strName,(in),const string_t&,val))
				OMEGA_METHOD_VOID(WriteGuid,2,((in),const string_t&,strName,(in),const guid_t&,val))

				OMEGA_METHOD_VOID(WriteBooleans,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const bool_t*,arr))
				OMEGA_METHOD_VOID(WriteBytes,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const byte_t*,arr))
				OMEGA_METHOD_VOID(WriteInt16s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const int16_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt16s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const uint16_t*,arr))
				OMEGA_METHOD_VOID(WriteInt32s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const int32_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt32s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const uint32_t*,arr))
				OMEGA_METHOD_VOID(WriteInt64s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const int64_t*,arr))
				OMEGA_METHOD_VOID(WriteUInt64s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const uint64_t*,arr))
				OMEGA_METHOD_VOID(WriteFloat4s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const float4_t*,arr))
				OMEGA_METHOD_VOID(WriteFloat8s,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const float8_t*,arr))
				OMEGA_METHOD_VOID(WriteStrings,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const string_t*,arr))
				OMEGA_METHOD_VOID(WriteGuids,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const guid_t*,arr))

				OMEGA_METHOD_VOID(WriteStructStart,2,((in),const string_t&,strName,(in),const string_t&,strType))
				OMEGA_METHOD_VOID(WriteStructEnd,1,((in),const string_t&,strName))
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IStub,

				OMEGA_METHOD_VOID(Invoke,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
				OMEGA_METHOD(bool_t,SupportsInterface,1,((in),const guid_t&,iid))
			)
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IStubController,

				OMEGA_METHOD_VOID(RemoteRelease,0,())
				OMEGA_METHOD(bool_t,RemoteQueryInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD_VOID(MarshalStub,2,((in),Remoting::IMessage*,pParamsIn,(in),Remoting::IMessage*,pParamsOut))
			)
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IProxy,

				OMEGA_METHOD_VOID(WriteKey,1,((in),Remoting::IMessage*,pMessage))
				OMEGA_METHOD_VOID(UnpackKey,1,((in),Remoting::IMessage*,pMessage))
				OMEGA_METHOD(Remoting::IMarshaller*,GetMarshaller,0,())
				OMEGA_METHOD(bool_t,IsAlive,0,())
				OMEGA_METHOD(bool_t,RemoteQueryInterface,1,((in),const guid_t&,iid))
				OMEGA_METHOD(IObject*,QueryIObject,0,())
			)
			
			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::Remoting, IMarshaller,

				OMEGA_METHOD_VOID(MarshalInterface,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),IObject*,pObject))
				OMEGA_METHOD_VOID(UnmarshalInterface,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
				OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in),IObject*,pObject))
				OMEGA_METHOD(Remoting::IMessage*,CreateMessage,0,())
				OMEGA_METHOD(IException*,SendAndReceive,4,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pSend,(out),Remoting::IMessage*&,pRecv,(in),uint32_t,timeout))
			)
			
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(byte_t,Byte)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(bool_t,Boolean)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int16_t,Int16)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint16_t,UInt16)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int32_t,Int32)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint32_t,UInt32)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(int64_t,Int64)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(uint64_t,UInt64)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(float4_t,Float4)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(float8_t,Float8)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(guid_t,Guid)
			OMEGA_WIRE_DECLARE_WIRE_READWRITE(string_t,String)

			template <typename T>
			struct std_wire_type
			{
				typedef typename remove_const<T>::type type;
								
				static void init(type&)
				{ }

				static void read(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, type& val)
				{
					wire_read(strName,pMessage,val);
				}

				static void write(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, typename optimal_param<T>::type val)
				{
					wire_write(strName,pMessage,val);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, typename optimal_param<T>::type)
				{
					// Just read the value back, moving the read pointer correctly
					type val = default_value<type>::value();
					wire_read(strName,pMessage,val);
				}

				static void no_op(bool)
				{ }
			};

			template <typename T>
			struct std_wire_type_array
			{
				struct type
				{
					type() : m_alloc_count(0),m_pVals(0)
					{}

					~type()
					{
						delete [] m_pVals;
					}

					void init(uint32_t count)
					{
						m_alloc_count = count;
						OMEGA_NEW(m_pVals,typename remove_const<T>::type[m_alloc_count]);
					}

					operator T*()
					{
						return m_pVals;
					}

					uint32_t                        m_alloc_count;
					typename remove_const<T>::type* m_pVals;
				};
				
				template <typename S>
				static void init(type& val, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					val.init(static_cast<uint32_t>(count));
				}

				template <typename S>
				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, T* pVals, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					wire_read(strName,pManager,pMessage,pVals,static_cast<uint32_t>(count));
				}

				template <typename S>
				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const T* pVals, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					wire_write(strName,pManager,pMessage,pVals,static_cast<uint32_t>(count));
				}

				template <typename S>
				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, T* pVals, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					wire_read(strName,pManager,pMessage,pVals,static_cast<uint32_t>(count));
				}

				template <typename S>
				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, type& val, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					val.init(static_cast<uint32_t>(count));
					wire_read(strName,pManager,pMessage,val.m_pVals,static_cast<uint32_t>(count));
				}

				template <typename S>
				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					// Only write back what we have room for...
					if (count > val.m_alloc_count)
						wire_write(strName,pManager,pMessage,val.m_pVals,val.m_alloc_count);
					else
						wire_write(strName,pManager,pMessage,val.m_pVals,static_cast<uint32_t>(count));
				}

				template <typename S>
				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, S count)
				{
					if (count > (uint32_t)-1 / sizeof(T))
						OMEGA_THROW(L"Attempt to marshal too many array items");

					// Only read back what we have written...
					if (count > val.m_alloc_count)
						wire_read(strName,pManager,pMessage,val.m_pVals,val.m_alloc_count);
					else
						wire_read(strName,pManager,pMessage,val.m_pVals,static_cast<uint32_t>(count));
				}				

				template <typename S>
				static void no_op(bool,S)
				{ }
			};
			
			template <typename T> 
			struct custom_wire_type_wrapper
			{
				typedef typename custom_wire_type<typename remove_const<T>::type>::impl impl;
				typedef typename impl::type type;
				typedef typename remove_const<T>::type raw_type;

				static void init(type& val)
				{
					impl::init(val);
				}

				static void init(type& val, const guid_t& iid)
				{
					impl::init(val,iid);
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, raw_type& val)
				{
					impl::read(strName,pManager,pMessage,val);
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, raw_type& val, const guid_t& iid)
				{
					impl::read(strName,pManager,pMessage,val,iid);
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, typename optimal_param<T>::type val)
				{
					impl::write(strName,pManager,pMessage,val);
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, typename optimal_param<T>::type val, const guid_t& iid)
				{
					impl::write(strName,pManager,pMessage,val,iid);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, typename optimal_param<T>::type val)
				{
					impl::unpack(strName,pManager,pMessage,val);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, typename optimal_param<T>::type val, const guid_t& iid)
				{
					impl::unpack(strName,pManager,pMessage,val,iid);
				}

				static void no_op(bool)
				{ }

				template <typename S>
				static void no_op(bool,S)
				{ }
			};

			template <typename I>
			struct iface_wire_type
			{
				struct type
				{
					type(I* val = 0) : m_val(val)
					{
						if (m_val)
							m_val->AddRef();
					}

					~type()
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

				static void init(type&, const guid_t& = OMEGA_GUIDOF(I))
				{ }

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, I*& pI, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					IObject* p = 0;
					pManager->UnmarshalInterface(strName,pMessage,iid,p);
					pI = static_cast<I*>(p);
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, I* pI, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					pManager->MarshalInterface(strName,pMessage,iid,pI);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, I* pI, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					pManager->ReleaseMarshalData(strName,pMessage,iid,pI);
				}
			};

			// STL collection marshalling types
			template <typename Coll>
			struct stl_safe_type_coll1
			{
				typedef const SafeShim* safe_type;

				typedef typename marshal_info<typename Coll::value_type>::safe_type impl;
				
				struct type_wrapper
				{
					type_wrapper(safe_type val) 
					{
						if (val)
						{
							auto_iface_ptr<Remoting::IMessage> msg = create_safe_proxy<Remoting::IMessage>(val);
							if (msg)
							{
								size_t count = 0;
								wire_read(string_t(),msg,count);
							
								for (size_t i=0;i<count;++i)
								{
									typename impl::type v = default_value<typename impl::type>::value();
									read(msg,v);
									m_val.insert(m_val.begin(),impl::coerce(v));
								}
							}
						}
					}

					void update(safe_type& val) 
					{
						// Ensure any existing shim is released
						auto_safe_shim shim = val;
						
						if (m_val.empty())
							val = 0;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							wire_write(string_t(),msg,m_val.size());
							for (typename Coll::reverse_iterator i=m_val.rbegin();i!=m_val.rend();++i)
								write(msg,static_cast<typename impl::type>(impl::clone(*i)));
							
							val = create_safe_stub(msg,OMEGA_GUIDOF(Remoting::IMessage));
						}
					}
					
					operator Coll&()
					{
						return m_val;
					}

				private:
					Coll m_val;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(const Coll& val)
					{
						if (val.empty())
							m_shim = 0;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							wire_write(string_t(),msg,val.size());
							for (typename Coll::const_reverse_iterator i=val.rbegin();i!=val.rend();++i)
								write(msg,static_cast<typename impl::type>(impl::coerce(*i)));

							m_shim = create_safe_stub(msg,OMEGA_GUIDOF(Remoting::IMessage));
						}
					}

					~safe_type_wrapper()
					{
						if (m_shim)
							release_safe(m_shim);
					}

					void update(Coll& dest) 
					{
						dest.clear();
						
						if (m_shim)
						{
							auto_iface_ptr<Remoting::IMessage> msg = create_safe_proxy<Remoting::IMessage>(m_shim);
							if (msg)
							{
								size_t count = 0;
								wire_read(string_t(),msg,count);
							
								for (size_t i=0;i<count;++i)
								{
									typename impl::type v = default_value<typename impl::type>::value();
									read(msg,v);
									dest.insert(dest.begin(),impl::clone(v));
								}
							}
						}
					}
					
					operator safe_type ()
					{
						return m_shim;
					}

					safe_type* operator & ()
					{
						return &m_shim;
					}

				private:
					const SafeShim* m_shim;
				};

				template <typename T>
				static void read(Remoting::IMessage* msg, T& val)
				{
					wire_read(string_t(),msg,val);
				}

				static void read(Remoting::IMessage* msg, guid_base_t& val)
				{
					guid_t g;
					wire_read(string_t(),msg,g);
					val = g;
				}

				template <typename T>
				static void read(Remoting::IMessage* msg, T*& pval)
				{
					uintptr_t ptr = 0;
					wire_read(string_t(),msg,ptr);
					pval = reinterpret_cast<T*>(ptr);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T val)
				{
					wire_write(string_t(),msg,val);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T* pval)
				{
					wire_write(string_t(),msg,reinterpret_cast<uintptr_t>(pval));
				}
			};

			// std::vector<bool> is badly broken in the C++ standard and shouldn't be used
			template <>
			struct stl_safe_type_coll1<std::vector<bool> >;
						
			template <typename Coll>
			struct stl_wire_type_coll1
			{
				typedef Coll type;

				static void read(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, Coll& val)
				{
					pMessage->ReadStructStart(strName,L"$collection_type_1");
					uint32_t count = pMessage->ReadUInt32(L"count");
					for (uint32_t c = 0;c<count;++c)
					{
						typename Coll::value_type v_val = default_value<typename Coll::value_type>::value();
						marshal_info<typename Coll::value_type>::wire_type::read((string_t(L"item{0}") % c),pMarshaller,pMessage,v_val);
						val.insert(val.end(),v_val);
					}
					pMessage->ReadStructEnd(strName);
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					if (val.size() > (uint32_t)-1 / sizeof(typename Coll::value_type))
						OMEGA_THROW(L"Attempt to marshal too many collection items");

					uint32_t count = static_cast<uint32_t>(val.size());

					pMessage->WriteStructStart(strName,L"$collection_type_1");
					pMessage->WriteUInt32(L"count",count);
					size_t idx = 0;
					for (typename Coll::const_iterator i=val.begin();i!=val.end();++i,++idx)
						marshal_info<typename Coll::value_type>::wire_type::write((string_t(L"item{0}") % idx),pMarshaller,pMessage,*i);
					pMessage->WriteStructEnd(strName);
				}
			};

			template <typename Coll>
			struct stl_safe_type_coll2
			{
				typedef const SafeShim* safe_type;

				typedef typename marshal_info<typename Coll::key_type>::safe_type key_impl;
				typedef typename marshal_info<typename Coll::mapped_type>::safe_type mapped_impl;
				
				struct type_wrapper
				{
					type_wrapper(safe_type val) 
					{
						if (val)
						{
							auto_iface_ptr<Remoting::IMessage> msg = create_safe_proxy<Remoting::IMessage>(val);
							if (msg)
							{
								size_t count = 0;
								wire_read(string_t(),msg,count);
							
								for (size_t i=0;i<count;++i)
								{
									typename key_impl::type k = default_value<typename key_impl::type>::value();
									typename mapped_impl::type m = default_value<typename mapped_impl::type>::value();
									read(msg,k);
									read(msg,m);
									m_val.insert(m_val.begin(),typename Coll::value_type(key_impl::coerce(k),mapped_impl::coerce(m)));
								}
							}
						}
					}

					void update(safe_type& val) 
					{
						// Ensure any existing shim is released
						auto_safe_shim shim = val;
						
						if (m_val.empty())
							val = 0;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							wire_write(string_t(),msg,m_val.size());
							for (typename Coll::reverse_iterator i=m_val.rbegin();i!=m_val.rend();++i)
							{
								write(msg,key_impl::clone(i->first));
								write(msg,mapped_impl::clone(i->second));
							}
							
							val = create_safe_stub(msg,OMEGA_GUIDOF(Remoting::IMessage));
						}
					}
					
					operator Coll&()
					{
						return m_val;
					}

				private:
					Coll m_val;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(const Coll& val)
					{
						if (val.empty())
							m_shim = 0;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							wire_write(string_t(),msg,val.size());
							for (typename Coll::const_reverse_iterator i=val.rbegin();i!=val.rend();++i)
							{
								write(msg,static_cast<typename key_impl::type>(key_impl::coerce(i->first)));
								write(msg,static_cast<typename mapped_impl::type>(mapped_impl::coerce(i->second)));
							}

							m_shim = create_safe_stub(msg,OMEGA_GUIDOF(Remoting::IMessage));
						}
					}

					~safe_type_wrapper()
					{
						if (m_shim)
							release_safe(m_shim);
					}

					void update(Coll& dest) 
					{
						dest.clear();
						
						if (m_shim)
						{
							auto_iface_ptr<Remoting::IMessage> msg = create_safe_proxy<Remoting::IMessage>(m_shim);
							if (msg)
							{
								size_t count = 0;
								wire_read(string_t(),msg,count);
							
								for (size_t i=0;i<count;++i)
								{
									typename key_impl::type k = default_value<typename key_impl::type>::value();
									typename mapped_impl::type m = default_value<typename mapped_impl::type>::value();
									read(msg,k);
									read(msg,m);
									dest.insert(dest.begin(),typename Coll::value_type(key_impl::clone(k),mapped_impl::clone(m)));
								}
							}
						}
					}
					
					operator safe_type ()
					{
						return m_shim;
					}

					safe_type* operator & ()
					{
						return &m_shim;
					}

				private:
					const SafeShim* m_shim;
				};

				template <typename T>
				static void read(Remoting::IMessage* msg, T& val)
				{
					wire_read(string_t(),msg,val);
				}

				static void read(Remoting::IMessage* msg, guid_base_t& val)
				{
					guid_t g;
					wire_read(string_t(),msg,g);
					val = g;
				}

				template <typename T>
				static void read(Remoting::IMessage* msg, T*& pval)
				{
					uintptr_t ptr = 0;
					wire_read(string_t(),msg,ptr);
					pval = reinterpret_cast<T*>(ptr);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T val)
				{
					wire_write(string_t(),msg,val);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T* pval)
				{
					wire_write(string_t(),msg,reinterpret_cast<uintptr_t>(pval));
				}
			};

			template <typename Coll>
			struct stl_wire_type_coll2
			{
				typedef Coll type;

				static void read(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, Coll& val)
				{
					pMessage->ReadStructStart(strName,L"$collection_type_2");
					uint32_t count = pMessage->ReadUInt32(L"count");
					for (uint32_t c = 0;c<count;++c)
					{
						typename Coll::key_type v_k = default_value<typename Coll::key_type>::value();
						marshal_info<typename Coll::key_type>::wire_type::read((string_t(L"key{0}") % c),pMarshaller,pMessage,v_k);

						typename Coll::mapped_type v_m = default_value<typename Coll::mapped_type>::value();
						marshal_info<typename Coll::mapped_type>::wire_type::read((string_t(L"item{0}") % c),pMarshaller,pMessage,v_m);

						val.insert(val.end(),typename Coll::value_type(v_k,v_m));
					}
					pMessage->ReadStructEnd(strName);
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					if (val.size() > (uint32_t)-1 / sizeof(typename Coll::value_type))
						OMEGA_THROW(L"Attempt to marshal too many collection items");

					uint32_t count = static_cast<uint32_t>(val.size());

					pMessage->WriteStructStart(strName,L"$collection_type_2");
					pMessage->WriteUInt32(L"count",count);
					size_t idx = 0;
					for (typename Coll::const_iterator i=val.begin();i!=val.end();++i,++idx)
					{
						marshal_info<typename Coll::key_type>::wire_type::write((string_t(L"key{0}") % idx),pMarshaller,pMessage,i->first);
						marshal_info<typename Coll::mapped_type>::wire_type::write((string_t(L"item{0}") % idx),pMarshaller,pMessage,i->second);
					}
					pMessage->WriteStructEnd(strName);
				}
			};

			template <typename V>
			struct custom_safe_type<std::list<V> >
			{
				typedef stl_safe_type_coll1<std::list<V> > impl;
			};
			
			template <typename V>
			struct custom_wire_type<std::list<V> >
			{
				typedef stl_wire_type_coll1<std::list<V> > impl;
			};
			
			template <typename V>
			struct custom_safe_type<std::vector<V> >
			{
				typedef stl_safe_type_coll1<std::vector<V> > impl;
			};

			template <typename V>
			struct custom_wire_type<std::vector<V> >
			{
				typedef stl_wire_type_coll1<std::vector<V> > impl;
			};

			template <typename V>
			struct custom_safe_type<std::set<V> >
			{
				typedef stl_safe_type_coll1<std::set<V> > impl;
			};

			template <typename V>
			struct custom_wire_type<std::set<V> >
			{
				typedef stl_wire_type_coll1<std::set<V> > impl;
			};

			template <typename K, typename V>
			struct custom_safe_type<std::map<K,V> >
			{
				typedef stl_safe_type_coll2<std::map<K,V> > impl;
			};

			template <typename K, typename V>
			struct custom_wire_type<std::map<K,V> >
			{
				typedef stl_wire_type_coll2<std::map<K,V> > impl;
			};

			template <typename K, typename V>
			struct custom_safe_type<std::multimap<K,V> >
			{
				typedef stl_safe_type_coll2<std::multimap<K,V> > impl;
			};

			template <typename K, typename V>
			struct custom_wire_type<std::multimap<K,V> >
			{
				typedef stl_wire_type_coll2<std::multimap<K,V> > impl;
			};
		}
	}
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_H_INCLUDED_
