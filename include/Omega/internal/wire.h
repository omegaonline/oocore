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
			virtual void ReadBytes(const string_t& strName, uint32_t count, byte_t* arr) = 0;
			virtual void WriteBytes(const string_t& strName, uint32_t count, const byte_t* arr) = 0;

			virtual any_t ReadValue(const string_t& strName) = 0;
			virtual void WriteValue(const string_t& strName, const any_t& value) = 0;

			virtual void ReadStructStart(const string_t& strName, const string_t& strType) = 0;
			virtual void ReadStructEnd() = 0;

			virtual void WriteStructStart(const string_t& strName, const string_t& strType) = 0;
			virtual void WriteStructEnd() = 0;

			virtual uint32_t ReadArrayStart(const string_t& strName) = 0;
			virtual void ReadArrayEnd() = 0;

			virtual void WriteArrayStart(const string_t& strName, uint32_t count) = 0;
			virtual void WriteArrayEnd() = 0;
		};

		inline IMessage* CreateMemoryMessage();
	
		interface IMarshaller : public IObject
		{
			virtual void MarshalInterface(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual void UnmarshalInterface(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject*& pObject) = 0;
			virtual void ReleaseMarshalData(const string_t& strName, IMessage* pMessage, const guid_t& iid, IObject* pObject) = 0;
			virtual IMessage* CreateMessage() = 0;
			virtual IException* SendAndReceive(TypeInfo::MethodAttributes_t attribs, IMessage* pSend, IMessage*& pRecv, uint32_t millisecs) = 0;
			virtual uint32_t GetSource() = 0;
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

				OMEGA_METHOD_VOID(ReadBytes,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),byte_t*,arr))
				OMEGA_METHOD_VOID(WriteBytes,3,((in),const string_t&,strName,(in),uint32_t,count,(in)(size_is(count)),const byte_t*,arr))

				OMEGA_METHOD(any_t,ReadValue,1,((in),const string_t&,strName))
				OMEGA_METHOD_VOID(WriteValue,2,((in),const string_t&,strName,(in),const any_t&,value))

				OMEGA_METHOD_VOID(ReadStructStart,2,((in),const string_t&,strName,(in),const string_t&,strType))
				OMEGA_METHOD_VOID(ReadStructEnd,0,())

				OMEGA_METHOD_VOID(WriteStructStart,2,((in),const string_t&,strName,(in),const string_t&,strType))
				OMEGA_METHOD_VOID(WriteStructEnd,0,())

				OMEGA_METHOD(uint32_t,ReadArrayStart,1,((in),const string_t&,strName))
				OMEGA_METHOD_VOID(ReadArrayEnd,0,())

				OMEGA_METHOD_VOID(WriteArrayStart,2,((in),const string_t&,strName,(in),uint32_t,count))
				OMEGA_METHOD_VOID(WriteArrayEnd,0,())
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

				OMEGA_METHOD_VOID(MarshalInterface,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD_VOID(UnmarshalInterface,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
				OMEGA_METHOD_VOID(ReleaseMarshalData,4,((in),const string_t&,strName,(in),Remoting::IMessage*,pMessage,(in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
				OMEGA_METHOD(Remoting::IMessage*,CreateMessage,0,())
				OMEGA_METHOD(IException*,SendAndReceive,4,((in),TypeInfo::MethodAttributes_t,attribs,(in),Remoting::IMessage*,pSend,(out),Remoting::IMessage*&,pRecv,(in),uint32_t,millisecs))
				OMEGA_METHOD(uint32_t,GetSource,0,())
			)

			template <typename T>
			struct std_wire_type
			{
				typedef typename remove_const<T>::type type;

				static void init(Remoting::IMarshaller*, type&)
				{ }

				static void read(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, type& val)
				{
					val = (pMessage->ReadValue(strName)).template cast<type>();
				}

				static void write(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, typename optimal_param<T>::type val)
				{
					pMessage->WriteValue(strName,val);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, typename optimal_param<T>::type)
				{
					// Just read the value back, moving the read pointer correctly
					pMessage->ReadValue(strName);
				}

				static void no_op(bool)
				{ }
			};

			template <typename T>
			struct std_wire_type_array
			{
				struct type
				{
					typedef typename remove_const<T>::type non_const_T;

					type() : m_alloc_count(0), m_pVals(NULL)
					{}

					~type()
					{
						if (m_pVals)
						{
							for (size_t i=0;i<m_alloc_count;++i)
								m_pVals[i].~non_const_T();

							::Omega::System::Free(m_pVals);
						}							
					}

					void init(Remoting::IMarshaller* /*pManager*/, size_t count)
					{
						// There is the potential for a remote DOS attack here
						// By sending a very large buffer to underpowered remote server
						// it might be possible to force it to choke on abscence of RAM
						//
						// The solution might be to have a throttling pool for allocation here...
						// Use pManager to detect who is doing what...
						//
						// Or maybe implement IMarshaller::Allocate()...
						
						
						if (count)
						{
							m_pVals = static_cast<non_const_T*>(::Omega::System::Allocate(count * sizeof(non_const_T)));

							size_t i = 0;
							try
							{
								for (;i<count;++i)
									::new (&m_pVals[i]) non_const_T();
							} 
							catch (...) 
							{ 
								for (;i > 0;--i)
									m_pVals[i-1].~non_const_T();
								
								::Omega::System::Free(m_pVals); 
								throw; 
							}
							m_alloc_count = count;							
						}
					}

					operator T*()
					{
						return m_pVals;
					}

					size_t       m_alloc_count;
					non_const_T* m_pVals;
				};

				static void init(Remoting::IMarshaller* pManager, type& val, size_t count)
				{
					val.init(pManager,count);
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, T* pVals, size_t count)
				{
					uint32_t count2 = pMessage->ReadArrayStart(strName);

					// Stop overflow
					if (count2 > count)
						count2 = static_cast<uint32_t>(count);

					if (pVals)
					{
						for (uint32_t c=0; c<count; ++c)
							marshal_info<T>::wire_type::read(string_t(),pManager,pMessage,pVals[c]);
					}

					pMessage->ReadArrayEnd();
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const T* pVals, size_t count)
				{
					pMessage->WriteArrayStart(strName,any_cast<uint32_t>(count));

					for (uint32_t c=0; c<count; ++c)
						marshal_info<T>::wire_type::write(string_t(),pManager,pMessage,pVals[c]);

					pMessage->WriteArrayEnd();
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const T* pVals, size_t count)
				{
					uint32_t count2 = pMessage->ReadArrayStart(strName);

					// Stop overflow
					if (count2 > count)
						count2 = static_cast<uint32_t>(count);

					for (uint32_t c=0; c<count2; ++c)
						marshal_info<T>::wire_type::unpack(string_t(),pManager,pMessage,pVals[c]);

					pMessage->ReadArrayEnd();
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, type& val, size_t)
				{
					uint32_t count = pMessage->ReadArrayStart(strName);

					val.init(pManager,count);

					for (uint32_t c=0; c<count; ++c)
						marshal_info<T>::wire_type::read(string_t(),pManager,pMessage,val.m_pVals[c]);

					pMessage->ReadArrayEnd();
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, size_t count)
				{
					// Only write out what we have allocated...
					if (count > val.m_alloc_count)
						count = val.m_alloc_count;

					pMessage->WriteArrayStart(strName,any_cast<uint32_t>(count));

					for (uint32_t c=0; c<count; ++c)
						marshal_info<T>::wire_type::write(string_t(),pManager,pMessage,val.m_pVals[c]);

					pMessage->WriteArrayEnd();
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, const type& val, size_t count)
				{
					uint32_t count2 = pMessage->ReadArrayStart(strName);

					OMEGA_UNUSED_ARG(count);

					// Stop overflow
					if (count2 > val.m_alloc_count)
						count2 = static_cast<uint32_t>(val.m_alloc_count);

					for (uint32_t c=0; c<count2; ++c)
						marshal_info<T>::wire_type::unpack(string_t(),pManager,pMessage,val.m_pVals[c]);

					pMessage->ReadArrayEnd();
				}

				static void no_op(bool, size_t = 0)
				{ }
			};

			template <typename T>
			struct custom_wire_type_wrapper
			{
				// If this line fails then you are attempting to marshal an unsupported type...
				typedef typename custom_wire_type<typename remove_const<T>::type>::impl impl;
				typedef typename impl::type type;
				typedef typename remove_const<T>::type raw_type;

				static void init(Remoting::IMarshaller* pManager, type& val)
				{
					impl::init(pManager,val);
				}

				static void init(Remoting::IMarshaller* pManager, type& val, const guid_t& iid)
				{
					impl::init(pManager,val,iid);
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
					type(I* val = NULL) : m_val(val)
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

				static void init(Remoting::IMarshaller*, type&, const guid_t& = OMEGA_GUIDOF(I))
				{ }

				static void read(const string_t& strName, Remoting::IMarshaller* pManager, Remoting::IMessage* pMessage, I*& pI, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					IObject* p = NULL;
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
								size_t count = msg->ReadValue(string_t()).template cast<size_t>();
								for (size_t i=0; i<count; ++i)
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
							val = NULL;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							msg->WriteValue(string_t(),m_val.size());
							for (typename Coll::reverse_iterator i=m_val.rbegin(); i!=m_val.rend(); ++i)
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
							m_shim = NULL;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							msg->WriteValue(string_t(),val.size());
							for (typename Coll::const_reverse_iterator i=val.rbegin(); i!=val.rend(); ++i)
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
								size_t count = msg->ReadValue(string_t()).template cast<size_t>();
								for (size_t i=0; i<count; ++i)
								{
									typename impl::type v = default_value<typename impl::type>::value();
									read(msg,v);
									dest.insert(dest.begin(),impl::clone(v));
								}
							}
						}
					}

					operator safe_type()
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
					val = msg->ReadValue(string_t()).template cast<T>();
				}

				static void read(Remoting::IMessage* msg, guid_base_t& val)
				{
					val = msg->ReadValue(string_t()).template cast<guid_t>();
				}

				static void read(Remoting::IMessage* msg, string_t::handle_t& val)
				{
					read(msg,val.p0);
					read(msg,val.p1);
				}

				template <typename T>
				static void read(Remoting::IMessage* msg, T*& pval)
				{
					uintptr_t ptr = msg->ReadValue(string_t()).template cast<uintptr_t>();
					pval = reinterpret_cast<T*>(ptr);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T val)
				{
					msg->WriteValue(string_t(),val);
				}

				static void write(Remoting::IMessage* msg, const guid_base_t& val)
				{
					msg->WriteValue(string_t(),guid_t(val));
				}

				static void write(Remoting::IMessage* msg, const string_t::handle_t& val)
				{
					write(msg,val.p0);
					write(msg,val.p1);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T* pval)
				{
					msg->WriteValue(string_t(),reinterpret_cast<uintptr_t>(pval));
				}
			};

			// std::vector<bool> is badly broken in the C++ standard and shouldn't be used
			template <typename A>
			struct stl_safe_type_coll1<std::vector<bool,A> >;

			template <typename Coll>
			struct stl_wire_type_coll1
			{
				typedef Coll type;

				static void init(Remoting::IMarshaller*, Coll&)
				{
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, Coll& val)
				{
					uint32_t count = pMessage->ReadArrayStart(strName);
					for (uint32_t c = 0; c<count; ++c)
					{
						typename Coll::value_type v_val = default_value<typename Coll::value_type>::value();
						marshal_info<typename Coll::value_type>::wire_type::read(string_t(),pMarshaller,pMessage,v_val);
						val.insert(val.end(),v_val);
					}
					pMessage->ReadArrayEnd();
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					pMessage->WriteArrayStart(strName,any_cast<uint32_t>(val.size()));

					for (typename Coll::const_iterator i=val.begin(); i!=val.end(); ++i)
						marshal_info<typename Coll::value_type>::wire_type::write(string_t(),pMarshaller,pMessage,*i);

					pMessage->WriteStructEnd();
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					uint32_t count = pMessage->ReadArrayStart(strName);

					for (typename Coll::const_iterator i=val.begin(); i!=val.end(); ++i)
						marshal_info<typename Coll::value_type>::wire_type::unpack(string_t(),pMarshaller,pMessage,*i);

					pMessage->ReadArrayEnd();
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
								size_t count = msg->ReadValue(string_t()).template cast<size_t>();
								for (size_t i=0; i<count; ++i)
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
							val = NULL;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							msg->WriteValue(string_t(),m_val.size());
							for (typename Coll::reverse_iterator i=m_val.rbegin(); i!=m_val.rend(); ++i)
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
							m_shim = NULL;
						else
						{
							auto_iface_ptr<Remoting::IMessage> msg = Remoting::CreateMemoryMessage();

							msg->WriteValue(string_t(),val.size());
							for (typename Coll::const_reverse_iterator i=val.rbegin(); i!=val.rend(); ++i)
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
								size_t count = msg->ReadValue(string_t()).template cast<size_t>();
								for (size_t i=0; i<count; ++i)
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

					operator safe_type()
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
					val = msg->ReadValue(string_t()).template cast<T>();
				}

				static void read(Remoting::IMessage* msg, guid_base_t& val)
				{
					val = msg->ReadValue(string_t()).template cast<guid_t>();
				}

				template <typename T>
				static void read(Remoting::IMessage* msg, T*& pval)
				{
					uintptr_t ptr = msg->ReadValue(string_t()).template cast<uintptr_t>();
					pval = reinterpret_cast<T*>(ptr);
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T val)
				{
					msg->WriteValue(string_t(),val);
				}

				static void write(Remoting::IMessage* msg, const guid_base_t& val)
				{
					msg->WriteValue(string_t(),guid_t(val));
				}

				template <typename T>
				static void write(Remoting::IMessage* msg, T* pval)
				{
					msg->WriteValue(string_t(),reinterpret_cast<uintptr_t>(pval));
				}
			};

			template <typename Coll>
			struct stl_wire_type_coll2
			{
				typedef Coll type;

				static void init(Remoting::IMarshaller*, Coll& val)
				{
				}

				static void read(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, Coll& val)
				{
					uint32_t count = pMessage->ReadArrayStart(strName);
					for (uint32_t c = 0; c<count; ++c)
					{
						pMessage->ReadStructStart(string_t::constant("pair"),string_t::constant("$pair_type"));

						typename Coll::key_type v_k = default_value<typename Coll::key_type>::value();
						marshal_info<typename Coll::key_type>::wire_type::read(string_t::constant("first"),pMarshaller,pMessage,v_k);

						typename Coll::mapped_type v_m = default_value<typename Coll::mapped_type>::value();
						marshal_info<typename Coll::mapped_type>::wire_type::read(string_t::constant("second"),pMarshaller,pMessage,v_m);

						val.insert(val.end(),typename Coll::value_type(v_k,v_m));

						pMessage->ReadStructEnd();
					}
					pMessage->ReadArrayEnd();
				}

				static void write(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					pMessage->WriteArrayStart(strName,any_cast<uint32_t>(val.size()));

					for (typename Coll::const_iterator i=val.begin(); i!=val.end(); ++i)
					{
						pMessage->WriteStructStart(string_t::constant("pair"),string_t::constant("$pair_type"));

						marshal_info<typename Coll::key_type>::wire_type::write(string_t::constant("first"),pMarshaller,pMessage,i->first);
						marshal_info<typename Coll::mapped_type>::wire_type::write(string_t::constant("second"),pMarshaller,pMessage,i->second);

						pMessage->WriteStructEnd();
					}
					pMessage->WriteArrayEnd();
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, const Coll& val)
				{
					uint32_t count = pMessage->ReadArrayStart(strName);

					for (typename Coll::const_iterator i=val.begin(); i!=val.end(); ++i)
					{
						pMessage->ReadStructStart(string_t::constant("pair"),string_t::constant("$pair_type"));

						marshal_info<typename Coll::key_type>::wire_type::unpack(string_t::constant("first"),pMarshaller,pMessage,i->first);
						marshal_info<typename Coll::mapped_type>::wire_type::unpack(string_t::constant("second"),pMarshaller,pMessage,i->second);

						pMessage->ReadStructEnd();
					}
					pMessage->ReadArrayEnd();
				}
			};

			template <>
			struct custom_wire_type<any_t>
			{
				typedef custom_wire_type<any_t> impl;
				typedef any_t type;

				static void init(Remoting::IMarshaller*, type&)
				{ }

				static void read(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, any_t& val)
				{
					val = pMessage->ReadValue(strName);
				}

				static void write(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, const any_t& val)
				{
					pMessage->WriteValue(strName,val);
				}

				static void unpack(const string_t& strName, Remoting::IMarshaller*, Remoting::IMessage* pMessage, const any_t&)
				{
					pMessage->ReadValue(strName);
				}
			};

			// We could probably do something clever with SFINAE here...
			template <typename T, typename A>
			struct custom_safe_type<std::vector<T,A> >
			{
				typedef stl_safe_type_coll1<std::vector<T,A> > impl;
			};

			template <typename T, typename A>
			struct custom_wire_type<std::vector<T,A> >
			{
				typedef stl_wire_type_coll1<std::vector<T,A> > impl;
			};

			template <typename T, typename A>
			struct custom_safe_type<std::deque<T,A> >
			{
				typedef stl_safe_type_coll1<std::deque<T,A> > impl;
			};

			template <typename T, typename A>
			struct custom_wire_type<std::deque<T,A> >
			{
				typedef stl_wire_type_coll1<std::deque<T,A> > impl;
			};

			template <typename T, typename A>
			struct custom_safe_type<std::list<T,A> >
			{
				typedef stl_safe_type_coll1<std::list<T,A> > impl;
			};

			template <typename T, typename A>
			struct custom_wire_type<std::list<T,A> >
			{
				typedef stl_wire_type_coll1<std::list<T,A> > impl;
			};

			template <typename T, typename P, typename A>
			struct custom_safe_type<std::set<T,P,A> >
			{
				typedef stl_safe_type_coll1<std::set<T,P,A> > impl;
			};

			template <typename T, typename P, typename A>
			struct custom_wire_type<std::set<T,P,A> >
			{
				typedef stl_wire_type_coll1<std::set<T,P,A> > impl;
			};

			template <typename T, typename P, typename A>
			struct custom_safe_type<std::multiset<T,P,A> >
			{
				typedef stl_safe_type_coll1<std::multiset<T,P,A> > impl;
			};

			template <typename T, typename P, typename A>
			struct custom_wire_type<std::multiset<T,P,A> >
			{
				typedef stl_wire_type_coll1<std::multiset<T,P,A> > impl;
			};

			template <typename K, typename V, typename P, typename A>
			struct custom_safe_type<std::map<K,V,P,A> >
			{
				typedef stl_safe_type_coll2<std::map<K,V,P,A> > impl;
			};

			template <typename K, typename V, typename P, typename A>
			struct custom_wire_type<std::map<K,V,P,A> >
			{
				typedef stl_wire_type_coll2<std::map<K,V,P,A> > impl;
			};

			template <typename K, typename V, typename P, typename A>
			struct custom_safe_type<std::multimap<K,V,P,A> >
			{
				typedef stl_safe_type_coll2<std::multimap<K,V,P,A> > impl;
			};

			template <typename K, typename V, typename P, typename A>
			struct custom_wire_type<std::multimap<K,V,P,A> >
			{
				typedef stl_wire_type_coll2<std::multimap<K,V,P,A> > impl;
			};
		}
	}
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_WIRE_H_INCLUDED_
