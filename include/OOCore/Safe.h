///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOCORE_SAFE_H_INCLUDED_
#define OOCORE_SAFE_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		inline bool PinObjectPointer(IObject* pObject);
		inline void UnpinObjectPointer(IObject* pObject);

		struct IProxy;

		namespace MetaInfo
		{
			template <typename T>
			struct std_safe_type
			{
				typedef T type;

				static T coerce(T val, ...)
				{
					return val;
				}
			};

			// MSVC gets twitchy about size_t
			#if defined(_MSC_VER) && defined(_Wp64)
			template <>
			struct std_safe_type<size_t>
			{
			#if defined(_M_IA64) || defined(_M_X64)
				typedef uint64_t type;
			#else
				typedef uint32_t type;
			#endif
				static type coerce(size_t val)
				{
					return static_cast<type>(val);
				}
			};
			#endif

			template <typename T>
			struct std_safe_type_ref
			{
				typedef T* type;

				static type coerce(T& val)
				{
					return &val;
				}

				static T& coerce(type val)
				{
					return *val;
				}
			};

			template <typename T> struct custom_safe_type;
			template <typename T> struct custom_wire_type;
			
			template <typename T> 
			struct custom_safe_type_wrapper
			{
				typedef typename custom_safe_type<T>::impl::safe_type type;

				static typename custom_safe_type<T>::impl::safe_type_wrapper coerce(T val)
				{
					return typename custom_safe_type<T>::impl::safe_type_wrapper(val);
				}

				static typename custom_safe_type<T>::impl::safe_type_wrapper coerce(T val, size_t count)
				{
					return typename custom_safe_type<T>::impl::safe_type_wrapper(val,count);
				}

				static typename custom_safe_type<T>::impl::type_wrapper coerce(type val)
				{
					return typename custom_safe_type<T>::impl::type_wrapper(val);
				}

				static typename custom_safe_type<T>::impl::type_wrapper coerce(type val, uint32_t count)
				{
					return typename custom_safe_type<T>::impl::type_wrapper(val,static_cast<size_t>(count));
				}

				static typename custom_safe_type<T>::impl::type_wrapper coerce(type val, const uint64_t& count)
				{
					return typename custom_safe_type<T>::impl::type_wrapper(val,static_cast<size_t>(count));
				}

				template <typename S>
				static typename custom_safe_type<T>::impl::type_wrapper coerce(type val, S* count)
				{
					return coerce(val,*count);
				}
			};

			template <typename T> 
			struct custom_safe_type_ref_wrapper
			{
				typedef typename custom_safe_type<T>::impl::safe_type* type;

				struct ref_holder
				{
					operator T&()
					{
						return m_val;
					}

				protected:
					typename custom_safe_type<T>::impl::type_wrapper m_val;
					type                                             m_dest;

					ref_holder(type val) : m_val(*val), m_dest(val)
					{}
				};

				struct ref_holder_lite : public ref_holder
				{
					ref_holder_lite(type val) :	ref_holder(val)
					{}

					~ref_holder_lite()
					{
						this->m_val.update(*this->m_dest);
					}
				};

				struct ref_holder_full : public ref_holder
				{
					ref_holder_full(type val, const guid_base_t* piid, const SafeShim* pOuter = 0) :
						ref_holder(val), m_piid(piid), m_pOuter(pOuter)
					{}

					~ref_holder_full()
					{
						this->m_val.update(*this->m_dest,m_piid,m_pOuter);
					}

				private:
					const guid_base_t* m_piid;
					const SafeShim*    m_pOuter;
				};

				struct ref_holder_safe
				{
					operator type()
					{
						return &m_val;
					}

				protected:
					typename custom_safe_type<T>::impl::safe_type_wrapper m_val;
					T&                                                    m_dest;
					
					ref_holder_safe(T& val) : m_val(val), m_dest(val)
					{}

					ref_holder_safe(const ref_holder_safe& rhs) : m_val(rhs.m_val), m_dest(rhs.m_dest)
					{}

				private:
					ref_holder_safe& operator = (const ref_holder_safe&);
				};

				struct ref_holder_safe_lite : public ref_holder_safe
				{
					ref_holder_safe_lite(T& val) : ref_holder_safe(val)
					{}

					~ref_holder_safe_lite()
					{
						this->m_val.update(this->m_dest);
					}
				};

				struct ref_holder_safe_full : public ref_holder_safe
				{
					ref_holder_safe_full(T& val, const guid_t& iid, IObject* pOuter = 0) : 
						ref_holder_safe(val), m_iid(iid), m_pOuter(pOuter)
					{}

					~ref_holder_safe_full()
					{
						this->m_val.update(this->m_dest,m_iid,m_pOuter);
					}

					ref_holder_safe_full(const ref_holder_safe_full& rhs) : ref_holder_safe(rhs), m_iid(rhs.m_iid), m_pOuter(rhs.m_pOuter)
					{}

				private:
					const guid_t& m_iid;
					IObject*      m_pOuter;

					ref_holder_safe_full& operator = (const ref_holder_safe_full&);
				};

				static ref_holder_safe_lite coerce(T& val)
				{
					return ref_holder_safe_lite(val);
				}

				static ref_holder_safe_full coerce(T& val, const guid_t& iid, IObject* pOuter = 0)
				{
					return ref_holder_safe_full(val,iid,pOuter);
				}

				static ref_holder_lite coerce(type val)
				{
					return ref_holder_lite(val);
				}

				static ref_holder_full coerce(type val, const guid_base_t* piid, const SafeShim* pOuter = 0)
				{
					return ref_holder_full(val,piid,pOuter);
				}
			};

			template <typename T> 
			struct custom_safe_type_const_ref_wrapper
			{
				typedef typename custom_safe_type<const T>::impl::safe_type* type;

				struct ref_holder_safe
				{
					ref_holder_safe(const T& val) : m_val(val)
					{}

					operator type()
					{
						return &m_val;
					}

				private:
					typename custom_safe_type<const T>::impl::safe_type_wrapper m_val;
				};

				static ref_holder_safe coerce(const T& val)
				{
					return ref_holder_safe(val);
				}

				static typename custom_safe_type<const T>::impl::type_wrapper coerce(type val)
				{
					return typename custom_safe_type<const T>::impl::type_wrapper(*val);
				}
			};

			// Don't pass arrays by const& - just pass the array
			template <typename T> 
			struct custom_safe_type_const_ref_wrapper<T*>;

			template <typename T> struct marshal_info;
			
			template <typename T> 
			struct array_safe_type
			{
				typedef typename marshal_info<T>::safe_type::type* safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type vals, size_t cbSize) : 
						m_bNull(!vals),
						m_val(default_value<T>::value()),
						m_pVals(0), 
						m_pOrig(vals), 
						m_cbSize(cbSize)
					{
						if (!m_bNull)
						{
							if (cbSize == 1)
								m_val = marshal_info<T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								OMEGA_NEW(m_pVals,T[cbSize]);
								for (size_t i=0;i<cbSize;++i)
									m_pVals[i] = marshal_info<T>::safe_type::coerce(vals[i]);
							}
						}
					}

					~type_wrapper()
					{
						if (!m_bNull)
						{
							if (m_pVals)
							{
								for (size_t i=0;i<m_cbSize;++i)
									m_pOrig[i] = marshal_info<T>::safe_type::coerce(m_pVals[i]);

								delete [] m_pVals;
							}
							else
								*m_pOrig = marshal_info<T>::safe_type::coerce(m_val);
						}						
					}

					operator T*()
					{
						if (m_bNull)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					bool      m_bNull;
					T         m_val;
					T*        m_pVals;
					safe_type m_pOrig;
					size_t    m_cbSize;
				};

				struct safe_type_wrapper
				{
					typedef typename marshal_info<T>::safe_type::type arr_type;

					safe_type_wrapper(T* vals, size_t cbSize) : 
						m_bNull(!vals),
						m_val(default_value<arr_type>::value()),
						m_pVals(0), 
						m_pOrig(vals), 
						m_cbSize(cbSize)
					{
						if (!m_bNull)
						{
							if (cbSize == 1)
								m_val = marshal_info<T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								OMEGA_NEW(m_pVals,arr_type[cbSize]);
								for (size_t i=0;i<cbSize;++i)
									m_pVals[i] = marshal_info<T>::safe_type::coerce(vals[i]);
							}
						}
					}

					~safe_type_wrapper()
					{
						if (!m_bNull)
						{
							if (m_pVals)
							{
								for (size_t i=0;i<m_cbSize;++i)
									m_pOrig[i] = marshal_info<T>::safe_type::coerce(m_pVals[i]);

								delete [] m_pVals;
							}
							else
								*m_pOrig = marshal_info<T>::safe_type::coerce(m_val);
						}	
					}

					operator safe_type()
					{
						if (m_bNull)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					bool      m_bNull;
					arr_type  m_val;
					arr_type* m_pVals;
					T*        m_pOrig;
					size_t    m_cbSize;
				};
			};

			template <typename T> 
			struct array_safe_type<const T>
			{
				typedef typename marshal_info<const T>::safe_type::type* safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type vals, size_t cbSize) : 
						m_bNull(!vals),
						m_val(default_value<T>::value()),
						m_pVals(0)
					{
						if (!m_bNull)
						{
							if (cbSize == 1)
								m_val = marshal_info<const T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								OMEGA_NEW(m_pVals,T[cbSize]);
								for (size_t i=0;i<cbSize;++i)
									m_pVals[i] = marshal_info<const T>::safe_type::coerce(vals[i]);
							}
						}
					}

					~type_wrapper()
					{
						if (m_pVals)
							delete [] m_pVals;
					}

					operator const T*()
					{
						if (m_bNull)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					bool m_bNull;
					T    m_val;
					T*   m_pVals;
				};

				struct safe_type_wrapper
				{
					typedef typename marshal_info<T>::safe_type::type arr_type;

					safe_type_wrapper(const T* vals, size_t cbSize) : 
						m_bNull(!vals),
						m_val(default_value<arr_type>::value()),
						m_pVals(0)
					{
						if (!m_bNull)
						{
							if (cbSize == 1)
								m_val = marshal_info<const T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								OMEGA_NEW(m_pVals,arr_type[cbSize]);
								for (size_t i=0;i<cbSize;++i)
									m_pVals[i] = marshal_info<const T>::safe_type::coerce(vals[i]);
							}
						}
					}

					~safe_type_wrapper()
					{
						if (m_pVals)
							delete [] m_pVals;
					}

					operator safe_type()
					{
						if (m_bNull)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					bool      m_bNull;
					arr_type  m_val;
					arr_type* m_pVals;
				};
			};

			template <typename T> 
			struct custom_safe_type<T*>
			{
				typedef array_safe_type<T> impl;
			};

			struct string_t_safe_type
			{
				typedef void* safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(string_t_safe_type::coerce(val))
					{
						string_t_safe_type::addref(val);
					}

					void update(safe_type& dest)
					{
						string_t_safe_type::release(dest);
						dest = string_t_safe_type::coerce(m_val);
						string_t_safe_type::addref(dest);
					}

					operator string_t&()
					{
						return m_val;
					}

				private:
					string_t m_val;
				};
				friend struct type_wrapper;

				struct safe_type_wrapper
				{
					safe_type_wrapper(const string_t& val) : m_val(string_t_safe_type::coerce(val))
					{
						string_t_safe_type::addref(m_val);
					}

					~safe_type_wrapper()
					{
						string_t_safe_type::release(m_val);
					}

					void update(string_t& dest)
					{
						string_t_safe_type::addref(m_val);
						dest = string_t_safe_type::coerce(m_val);
					}

					operator safe_type ()
					{
						return m_val;
					}

					safe_type* operator & ()
					{
						return &m_val;
					}

				private:
					safe_type m_val;
				};
				friend struct safe_type_wrapper;

			private:
				static void* coerce(const string_t& s)
				{
					return static_cast<void*>(s.m_handle);
				}

				static string_t coerce(void* v)
				{
					return string_t(static_cast<string_t::handle_t*>(v));
				}

				static void addref(void* v)
				{
					string_t::addref(static_cast<string_t::handle_t*>(v));
				}

				static void release(void* v)
				{
					string_t::release(static_cast<string_t::handle_t*>(v));
				}
			};

			template <>
			struct custom_safe_type<string_t>
			{
				typedef struct string_t_safe_type impl;
			};

			template <>
			struct custom_safe_type<const string_t>
			{
				typedef struct string_t_safe_type impl;
			};

			template <>
			struct custom_safe_type<bool_t>
			{
				typedef custom_safe_type<bool_t> impl;
				typedef int safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(val != 0)
					{}

					void update(safe_type& dest)
					{
						dest = (m_val ? 1 : 0);
					}

					operator bool_t&()
					{
						return m_val;
					}

				private:
					bool_t m_val;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(bool_t val = false) : m_val(val ? 1 : 0)
					{}

					void update(bool_t& dest)
					{
						dest = (m_val != 0);
					}

					operator safe_type ()
					{
						return m_val;
					}

					safe_type* operator & ()
					{
						return &m_val;
					}

				private:
					safe_type m_val;
				};
			};

			template <>
			struct custom_safe_type<const bool_t>
			{
				typedef custom_safe_type<const bool_t> impl;
				typedef const int safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(val != 0)
					{}

					operator const bool_t&()
					{
						return m_val;
					}

				private:
					bool_t m_val;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(const bool_t val = false) : m_val(val ? 1 : 0)
					{}

					safe_type_wrapper(const safe_type_wrapper& rhs) : m_val(rhs.m_val)
					{}

					operator safe_type ()
					{
						return m_val;
					}

					safe_type* operator & ()
					{
						return &m_val;
					}

				private:
					safe_type m_val;

					safe_type_wrapper& operator = (const safe_type_wrapper&);
				};
			};

			template <>
			struct custom_safe_type<guid_t>
			{
				typedef custom_safe_type<guid_t> impl;
				typedef guid_base_t safe_type;

				struct type_wrapper
				{
					type_wrapper(const safe_type& val) : m_val(val)
					{}

					void update(safe_type& dest)
					{
						dest = m_val;
					}

					operator guid_t&()
					{
						return m_val;
					}

				private:
					guid_t m_val;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(guid_t& val) : m_val(&val)
					{}

					void update(guid_t&)
					{}

					operator safe_type ()
					{
						return *m_val;
					}

					safe_type* operator & ()
					{
						return m_val;
					}

				private:
					safe_type* m_val;
				};
			};

			template <>
			struct custom_safe_type<const guid_t>
			{
				typedef custom_safe_type<const guid_t> impl;
				typedef const guid_base_t safe_type;

				typedef guid_t type_wrapper;
				
				struct safe_type_wrapper
				{
					safe_type_wrapper(const guid_t& val) : m_val(&val)
					{}

					operator safe_type ()
					{
						return *m_val;
					}

					safe_type* operator & ()
					{
						return m_val;
					}

				private:
					safe_type* m_val;
				};
			};

			template <typename T> struct is_message_type
			{
				enum { result = 0 };
			};

			// These are the types that can be natively marshalled
			template <> struct is_message_type<bool_t> { enum { result = 1 }; };
			template <> struct is_message_type<byte_t> { enum { result = 1 }; };
			template <> struct is_message_type<int16_t> { enum { result = 1 }; };
			template <> struct is_message_type<uint16_t> { enum { result = 1 }; };
			template <> struct is_message_type<int32_t> { enum { result = 1 }; };
			template <> struct is_message_type<uint32_t> { enum { result = 1 }; };
			template <> struct is_message_type<int64_t> { enum { result = 1 }; };
			template <> struct is_message_type<uint64_t> { enum { result = 1 }; };
			template <> struct is_message_type<float4_t> { enum { result = 1 }; };
			template <> struct is_message_type<float8_t> { enum { result = 1 }; };
			template <> struct is_message_type<string_t> { enum { result = 1 }; };
			template <> struct is_message_type<guid_t> { enum { result = 1 }; };

			template <typename T> struct is_message_type<const T>
			{
				enum { result = is_message_type<T>::result };
			};

			template <typename T> struct std_wire_type;
			template <typename T> struct std_wire_type_array;

			template <typename T> struct custom_wire_type_wrapper;
			template <typename T> struct custom_wire_type_ref_wrapper;

			template <typename T> struct marshal_info
			{
				typedef typename if_else_t<
					is_c_abi<T>::result,
					std_safe_type<T>,
					custom_safe_type_wrapper<T> 
				>::result safe_type;
				
				typedef typename if_else_t<
					is_message_type<T>::result,
					std_wire_type<T>,
					custom_wire_type_wrapper<T> 
				>::result wire_type;
			};

			template <typename T> struct marshal_info<T&>
			{
				typedef typename if_else_t<
					is_c_abi<T>::result,
					std_safe_type_ref<T>,
					custom_safe_type_ref_wrapper<T>
				>::result safe_type;

				typedef typename if_else_t<
					is_message_type<T>::result,
					std_wire_type<T&>,
					custom_wire_type_wrapper<T>
				>::result wire_type;
			};

			template <typename T> struct marshal_info<const T&>
			{
				typedef typename if_else_t<
					is_c_abi<const T>::result,
					std_safe_type_ref<const T>,
					custom_safe_type_const_ref_wrapper<T>
				>::result safe_type;

				typedef typename if_else_t<
					is_message_type<const T>::result,
					std_wire_type<const T&>,
					custom_wire_type_wrapper<const T>
				>::result wire_type;
			};

			template <typename T> struct marshal_info<T*>
			{
				typedef typename if_else_t<
					is_c_abi<T*>::result,
					std_safe_type<T*>,
					custom_safe_type_wrapper<T*>
				>::result safe_type;

				typedef typename if_else_t<
					is_message_type<T>::result,
					std_wire_type_array<T>,
					custom_wire_type_wrapper<T*>
				>::result wire_type;
			};

			struct SafeShim
			{
				const void* m_vtable;
				void* m_stub;
				const guid_base_t* m_iid;
			};

			struct ISafeProxy : public IObject
			{
				virtual void Pin() = 0;
				virtual void Unpin() = 0;
				virtual const SafeShim* GetShim(const Omega::guid_t& iid) = 0;
				virtual const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid) = 0;
				virtual IProxy* GetWireProxy() = 0;
			};

			inline void throw_correct_exception(const SafeShim* except);
			inline const SafeShim* return_safe_exception(IException* pE);

			struct IObject_Safe_VTable
			{
				const SafeShim* (OMEGA_CALL* pfnAddRef_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnRelease_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnQueryInterface_Safe)(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid);
				const SafeShim* (OMEGA_CALL* pfnPin_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnUnpin_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnGetBaseShim_Safe)(const SafeShim* shim, const SafeShim** retval);
				const SafeShim* (OMEGA_CALL* pfnCreateWireStub_Safe)(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_base_t* piid, const SafeShim** retval);
				const SafeShim* (OMEGA_CALL* pfnGetWireProxy_Safe)(const SafeShim* shim, const SafeShim** retval);
			};

			template <typename I>
			class auto_iface_ptr
			{
			public:
				auto_iface_ptr(I* pI = 0) : m_pI(pI)
				{}

				auto_iface_ptr(const auto_iface_ptr& rhs) : m_pI(rhs.m_pI)
				{
					AddRef();
				}

				auto_iface_ptr& operator = (const auto_iface_ptr& rhs)
				{
					if (&rhs != this)
					{
						Release();

						m_pI = rhs.m_pI;
						
						AddRef();
					}
					return *this;
				}

				~auto_iface_ptr()
				{
					Release();
				}

				operator I*&()
				{
					return m_pI;
				}

				I* operator ->() const
				{
					return m_pI;
				}

				void attach(I* pI)
				{
					Release();
					m_pI = pI;
				}

			private:
				void AddRef()
				{
					if (m_pI)
						m_pI->AddRef();
				}

				void Release()
				{
					if (m_pI)
						m_pI->Release();
				}

				I* m_pI;
			};

			class auto_safe_shim
			{
			public:
				auto_safe_shim(const SafeShim* pS = 0) : m_pS(pS)
				{}

				auto_safe_shim(const auto_safe_shim& rhs) : m_pS(rhs.m_pS)
				{
					AddRef();
				}

				auto_safe_shim& operator = (const auto_safe_shim& rhs)
				{
					if (&rhs != this)
					{
						Release();

						m_pS = rhs.m_pS;
						
						AddRef();
					}
					return *this;
				}

				~auto_safe_shim()
				{
					Release();
				}

				operator const SafeShim*&()
				{
					return m_pS;
				}

				const SafeShim** operator &()
				{
					return &m_pS;
				}

				const SafeShim* operator ->()
				{
					return m_pS;
				}

				void attach(const SafeShim* pS)
				{
					Release();
					m_pS = pS;
				}

				const SafeShim* detach()
				{
					const SafeShim* ret = m_pS;
					m_pS = 0;
					return ret;
				}

			private:
				const SafeShim* m_pS;

				void AddRef()
				{
					if (m_pS)
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnAddRef_Safe(m_pS);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Release()
				{
					if (m_pS)
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnRelease_Safe(m_pS);
						if (except)
							throw_correct_exception(except);
					}
				}
			};

			inline IObject* create_safe_proxy(const SafeShim* shim, IObject* pOuter = 0);
			inline const SafeShim* create_safe_stub(IObject* pObject, const guid_t& iid);

			template <typename I>
			class iface_safe_type
			{
			public:
				typedef const SafeShim* safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type pS) :
						m_pI(0)
					{
						m_pI = static_cast<I*>(create_safe_proxy(pS));
					}

					~type_wrapper()
					{
						if (m_pI)
							m_pI->Release();
					}

					operator I*&()
					{
						return m_pI;
					}

					void update(safe_type& pS, const guid_base_t* piid = 0, const SafeShim* = 0)
					{
						if (pS)
							static_cast<const IObject_Safe_VTable*>(pS->m_vtable)->pfnRelease_Safe(pS);
						
						pS = create_safe_stub(this->m_pI,piid ? *piid : OMEGA_GUIDOF(I));
					}

				private:
					I* m_pI;
				};

				struct safe_type_wrapper
				{
					safe_type_wrapper(I* pI)
					{
						m_pS = create_safe_stub(pI,OMEGA_GUIDOF(I));
					}

					~safe_type_wrapper()
					{
						if (m_pS)
							static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnRelease_Safe(m_pS);
					}

					operator safe_type()
					{
						return m_pS;
					}

					safe_type* operator & ()
					{
						return &m_pS;
					}

					void update(I*& pI)
					{
						if (pI)
							pI->Release();

						pI = static_cast<I*>(create_safe_proxy(m_pS,0));
					}

					void update(I*& pI, const guid_t& /*iid*/, IObject* pOuter = 0)
					{
						if (pI)
							pI->Release();

						pI = static_cast<I*>(create_safe_proxy(m_pS,pOuter));
					}

				private:
					safe_type m_pS;
				};
			};

			template <typename I> struct iface_wire_type;

			template <> struct custom_safe_type<IObject*>
			{
				typedef iface_safe_type<IObject> impl;
			};

			template <> struct custom_wire_type<IObject*>
			{
				typedef iface_wire_type<IObject> impl;
			};

			template <typename I> struct vtable_info;

			template <> struct vtable_info<IObject>
			{
				typedef IObject_Safe_VTable type;
			};

			class Safe_Proxy_Owner;
			
			class Safe_Proxy_Base
			{
			public:
				virtual bool IsDerived__proxy__(const guid_t& iid) const = 0;
				virtual IObject* QIReturn__proxy__() = 0;
				virtual void Throw__proxy__() = 0;

				void AddRef()
				{
					if (m_refcount.AddRef())
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnRelease_Safe(m_shim);
						if (except)
							throw_correct_exception(except);

						if (m_intcount.IsZero() && m_pincount.IsZero())
							delete this;
					}
				}

				inline IObject* QueryInterface(const guid_t& iid);

				void Pin()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnPin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnUnpin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					if (m_pincount.Release() && m_intcount.IsZero() && m_refcount.IsZero())
						delete this;
				}

				inline const SafeShim* GetShim(const Omega::guid_t& iid);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid);
				
				const SafeShim* GetShim()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				
					return m_shim;
				}

				void Internal_AddRef()
				{
					m_intcount.AddRef();
				}

				void Internal_Release()
				{
					assert(m_intcount.m_debug_value > 0);

					if (m_intcount.Release() && m_refcount.IsZero() && m_pincount.IsZero())
						delete this;
				}

			protected:
				Safe_Proxy_Base(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : m_shim(shim), m_pOwner(pOwner)
				{
					m_internal.m_pOuter = this;
					AddRef();
				}

				inline virtual ~Safe_Proxy_Base();

				inline void Throw(const guid_t& iid);

				const SafeShim* m_shim;
				
			private:
				Safe_Proxy_Base(const Safe_Proxy_Base&);
				Safe_Proxy_Base& operator =(const Safe_Proxy_Base&);	

				Safe_Proxy_Owner*         m_pOwner;
				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_intcount;
				Threading::AtomicRefCount m_pincount;

				struct Internal : public ISafeProxy
				{
					void AddRef() 
					{
						m_pOuter->Internal_AddRef(); 
					}

					void Release() 
					{
						m_pOuter->Internal_Release(); 
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOuter->QueryInterface(iid);
					}

					void Pin()
					{
						m_pOuter->Pin();
					}

					void Unpin()
					{
						m_pOuter->Unpin();
					}

					const SafeShim* GetShim(const Omega::guid_t& iid)
					{
						return m_pOuter->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
					{
						return m_pOuter->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					IProxy* GetWireProxy()
					{
						return 0;
					}

					Safe_Proxy_Base* m_pOuter;
				};
				Internal m_internal;
			};

			template <typename I, typename D>
			class Safe_Proxy;

			template <typename D>
			class Safe_Proxy<IObject,D> : public Safe_Proxy_Base, public D
			{
			public:
				static Safe_Proxy_Base* bind(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
				{
					Safe_Proxy* pThis;
					OMEGA_NEW(pThis,Safe_Proxy(shim,pOwner));
					return pThis;
				}

			protected:
				Safe_Proxy(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : 
					 Safe_Proxy_Base(shim,pOwner)
				{}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

			private:
				IObject* QIReturn__proxy__()
				{
					AddRef();
					return static_cast<D*>(this);
				}

				void Throw__proxy__()
				{
					throw static_cast<D*>(this);
				}
	
			// IObject members
			public:
				virtual void AddRef()
				{
					Safe_Proxy_Base::AddRef();
				}

				virtual void Release()
				{
					Safe_Proxy_Base::Release();
				}

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					if (iid != OMEGA_GUIDOF(IObject) && IsDerived__proxy__(iid))
						return QIReturn__proxy__();
					
					return Safe_Proxy_Base::QueryInterface(iid);
				}
			};

			class Safe_Stub_Owner;

			class Safe_Stub_Base
			{
			public:
				void AddRef()
				{
					if (m_refcount.AddRef())
						m_pI->AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
					{
						m_pI->Release();

						if (m_pincount.IsZero())
							delete this;
					}
				}

				const SafeShim* GetShim()
				{
					AddRef();
					return &m_shim;
				}

				virtual bool IsDerived(const guid_t& iid) const = 0;
				
			protected:
				SafeShim m_shim;
				IObject* m_pI;
				
				Safe_Stub_Base(IObject* pI, Safe_Stub_Owner* pOwner) : 
					 m_pI(pI), m_pOwner(pOwner)
				{
					AddRef(); 
				}

				inline virtual ~Safe_Stub_Base();

				inline const SafeShim* QueryInterface(const guid_t& iid);

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero())
						delete this;
				}

				inline const SafeShim* GetBaseShim();
								
			private:
				Safe_Stub_Owner*          m_pOwner;
				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_pincount;

				Safe_Stub_Base(const Safe_Stub_Base&);
				Safe_Stub_Base& operator =(const Safe_Stub_Base&);
			};

			template <typename I>
			class Safe_Stub;

			template <>
			class Safe_Stub<IObject> : public Safe_Stub_Base
			{
			public:
				static Safe_Stub_Base* create(IObject* pI, Safe_Stub_Owner* pOwner)
				{
					Safe_Stub* pThis;
					OMEGA_NEW(pThis,Safe_Stub(pI,OMEGA_GUIDOF(IObject),pOwner));
					return pThis;					
				}

			protected:
				Safe_Stub(IObject* pI, const guid_t& iid, Safe_Stub_Owner* pOwner) : 
					 Safe_Stub_Base(pI,pOwner)
				{
					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = &iid;
				}

				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&Pin_Safe,
						&Unpin_Safe,
						&GetBaseShim_Safe,
						0,
						0
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}
				
			private:
				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Release_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->QueryInterface(*iid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Pin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Pin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Unpin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Unpin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetBaseShim_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			class Safe_Proxy_Owner
			{
			public:
				Safe_Proxy_Owner(const SafeShim* shim, IObject* pOuter) : m_base_shim(shim), m_pOuter(pOuter)
				{
					// Pin the base_shim
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnPin_Safe(m_base_shim);
					if (except)
						throw_correct_exception(except);

					m_internal.m_pOwner = this;	
					m_safe_proxy.m_pOwner = this;	
					AddRef();
				}

				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release() && m_pincount.IsZero() && ListEmpty())
						delete this;
				}

				inline void RemoveBase(Safe_Proxy_Base* pProxy);
				inline IObject* QueryInterface(const guid_t& iid);
				inline IObject* CreateProxy(const SafeShim* shim);
				inline void Throw(const SafeShim* shim);
				inline void Throw(const guid_t& iid);
				inline const SafeShim* GetShim(const guid_t& iid);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);

			private:
				Safe_Proxy_Owner(const Safe_Proxy_Owner&);
				Safe_Proxy_Owner& operator =(const Safe_Proxy_Owner&);

				Threading::Mutex                  m_lock;
				std::map<guid_t,Safe_Proxy_Base*> m_iid_map;
				const SafeShim*                   m_base_shim;
				IObject*                          m_pOuter;
				Threading::AtomicRefCount         m_refcount;
				Threading::AtomicRefCount         m_pincount;

				struct Internal : public IObject
				{
					void AddRef()
					{
						if (m_refcount.AddRef())
						{
							m_pOwner->AddRef();

							const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pOwner->m_base_shim->m_vtable)->pfnAddRef_Safe(m_pOwner->m_base_shim);
							if (except)
								throw_correct_exception(except);
						}
					}

					void Release()
					{
						assert(m_refcount.m_debug_value > 0);

						if (m_refcount.Release())
						{
							const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pOwner->m_base_shim->m_vtable)->pfnRelease_Safe(m_pOwner->m_base_shim);
							if (except)
								throw_correct_exception(except);

							m_pOwner->Release();
						}
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					Safe_Proxy_Owner* m_pOwner;

				private:
					Threading::AtomicRefCount m_refcount;
				};
				Internal m_internal;

				struct SafeProxy : public ISafeProxy
				{
					void AddRef()
					{
						m_pOwner->AddRef();
					}

					void Release()
					{
						m_pOwner->Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					void Pin()
					{
						m_pOwner->Pin();
					}

					void Unpin()
					{
						m_pOwner->Unpin();
					}

					const SafeShim* GetShim(const Omega::guid_t& iid)
					{
						return m_pOwner->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
					{
						return m_pOwner->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					IProxy* GetWireProxy()
					{
						return 0;
					}

					Safe_Proxy_Owner* m_pOwner;
				};
				SafeProxy m_safe_proxy;
								
				inline virtual ~Safe_Proxy_Owner();

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero() && ListEmpty())
						delete this;
				}

				bool ListEmpty()
				{
					try
					{
						Threading::Guard<Threading::Mutex> guard(m_lock);
						return m_iid_map.empty();
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}
				}

				inline auto_iface_ptr<Safe_Proxy_Base> GetProxyBase(const guid_t& iid, const SafeShim* shim, bool bAllowPartial);
			};

			inline Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Owner> create_safe_proxy_owner(const SafeShim* shim, IObject* pOuter);
			
			class Safe_Stub_Owner
			{
			public:
				Safe_Stub_Owner(IObject* pObject) :
					m_pI(pObject)
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&Pin_Safe,
						&Unpin_Safe,
						&GetBaseShim_Safe,
						&CreateWireStub_Safe,
						0
					};
					m_base_shim.m_vtable = &vt;
					m_base_shim.m_stub = this;
					m_base_shim.m_iid = &OMEGA_GUIDOF(IObject);

					AddRef();
				}

				virtual void AddRef()
				{
					if (m_refcount.AddRef())
						m_pI->AddRef();
				}

				virtual void Release()
				{
					assert(m_refcount.m_debug_value > 0);
					
					if (m_refcount.Release())
					{
						m_pI->Release();

						if (m_pincount.IsZero() && ListEmpty())
							delete this;
					}
				}

				inline const SafeShim* QueryInterface(const guid_t& iid, IObject* pObj);

				const SafeShim* GetBaseShim()
				{
					AddRef();
					return &m_base_shim;
				}

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero() && ListEmpty())
						delete this;
				}

				inline void RemoveBase(Safe_Stub_Base* pStub);
				
			private:
				Safe_Stub_Owner(const Safe_Stub_Owner&);
				Safe_Stub_Owner& operator =(const Safe_Stub_Owner&);

				Threading::Mutex                 m_lock;
				std::map<guid_t,Safe_Stub_Base*> m_iid_map;
				IObject*                         m_pI;
				Threading::AtomicRefCount        m_refcount;
				Threading::AtomicRefCount        m_pincount;
				SafeShim                         m_base_shim;

				inline virtual ~Safe_Stub_Owner();

				inline auto_iface_ptr<Safe_Stub_Base> GetStubBase(const guid_t& iid, IObject* pObj);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);

				bool ListEmpty()
				{
					try
					{
						Threading::Guard<Threading::Mutex> guard(m_lock);
						return m_iid_map.empty();
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}
				}

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Release_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->QueryInterface(*iid,0);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Pin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Pin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Unpin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Unpin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetBaseShim_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL CreateWireStub_Safe(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_base_t* piid, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->CreateWireStub(shim_Controller,shim_Marshaller,*piid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			inline auto_iface_ptr<Safe_Stub_Owner> create_safe_stub_owner(IObject* pObject);

			struct OMEGA_PRIVATE_TYPE(safe_module)
			{
				int unused;
			};

			class safe_proxy_holder
			{
			public:
				inline void remove(const SafeShim* shim);
				inline auto_iface_ptr<Safe_Proxy_Owner> find(const SafeShim* shim);
				inline auto_iface_ptr<Safe_Proxy_Owner> add(const SafeShim* shim, Safe_Proxy_Owner* pOwner);

			private:
				Threading::Mutex                            m_lock;
				std::map<const SafeShim*,Safe_Proxy_Owner*> m_map;
			};
			typedef Threading::Singleton<safe_proxy_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > SAFE_PROXY_HOLDER;

			class safe_stub_holder
			{
			public:
				inline void remove(IObject* pObject);
				inline auto_iface_ptr<Safe_Stub_Owner> find(IObject* pObject);
				inline auto_iface_ptr<Safe_Stub_Owner> add(IObject* pObject, Safe_Stub_Owner* pOwner);

			private:
				Threading::Mutex                    m_lock;
				std::map<IObject*,Safe_Stub_Owner*> m_map;
			};
			typedef Threading::Singleton<safe_stub_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > SAFE_STUB_HOLDER;

			struct qi_rtti
			{
				Safe_Proxy_Base* (*pfnCreateSafeProxy)(const SafeShim* shim, Safe_Proxy_Owner* pOwner);
				Safe_Stub_Base* (*pfnCreateSafeStub)(IObject* pI, Safe_Stub_Owner* pOwner);
				const wchar_t* pszName;
			};

			typedef Threading::Singleton<std::map<guid_t,const qi_rtti*>,Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > RTTI_HOLDER;

			inline static const qi_rtti* get_qi_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const qi_rtti*>* iid_map = RTTI_HOLDER::instance();
					std::map<guid_t,const qi_rtti*>::const_iterator i=iid_map->find(iid);
					if (i != iid_map->end())
						return i->second;
				}
				catch (...)
				{}
				
				return 0;
			}

			inline static void register_rtti_info(const guid_t& iid, const qi_rtti* pRtti)
			{
				try
				{
					RTTI_HOLDER::instance()->insert(std::map<guid_t,const qi_rtti*>::value_type(iid,pRtti));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}
			}

			OMEGA_DECLARE_FORWARDS(Omega,IException)
			OMEGA_DECLARE_FORWARDS(Omega::TypeInfo,ITypeInfo)

			OMEGA_DEFINE_INTERNAL_INTERFACE_NOPROXY
			(
				Omega, IException,

				OMEGA_METHOD_VOID(Throw,0,())
				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
				OMEGA_METHOD(string_t,GetSource,0,())
			)

			template <typename D>
			class Safe_Proxy<IException,D> : public Safe_Proxy<IObject,D>
			{
				const vtable_info<Omega::IException>::type* deref_vt() 
				{ 
					return static_cast<const vtable_info<IException>::type*>(this->m_shim->m_vtable); 
				}

			public:
				static Safe_Proxy_Base* bind(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
				{
					Safe_Proxy* pThis; 
					OMEGA_NEW(pThis,Safe_Proxy(shim,pOwner)); 
					return pThis;
				}

			protected:
				Safe_Proxy(const SafeShim* shim, Safe_Proxy_Owner* pOwner = 0) : 
					 Safe_Proxy<IObject,D>(shim,pOwner) 
				{}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					if (iid == OMEGA_GUIDOF(Omega::IException)) 
						return true;
					return Safe_Proxy<IObject,D>::IsDerived__proxy__(iid);
				}

			public:
				void Throw()
				{
					// Make sure we relase our refcount before throwing the correct interface
					auto_iface_ptr<Safe_Proxy<IException,D> > ptrThis(this);

					Safe_Proxy_Base::Throw(GetThrownIID());
				}

				guid_t GetThrownIID()
				{
					guid_t retval;
					const SafeShim* pE = deref_vt()->pfnGetThrownIID_Safe(this->m_shim,&retval);
					if (pE)
						throw_correct_exception(pE);
					return retval;
				}

				IException* GetCause()
				{
					IException* retval = 0;
					const SafeShim* pE = deref_vt()->pfnGetCause_Safe(this->m_shim,marshal_info<IException*&>::safe_type::coerce(retval));
					if (pE)
						throw_correct_exception(pE);
					return retval;
				}

				string_t GetDescription()
				{
					string_t retval;
					const SafeShim* pE = deref_vt()->pfnGetDescription_Safe(this->m_shim,marshal_info<string_t&>::safe_type::coerce(retval));
					if (pE)
						throw_correct_exception(pE);
					return retval;
				}

				string_t GetSource()
				{
					string_t retval;
					const SafeShim* pE = deref_vt()->pfnGetSource_Safe(this->m_shim,marshal_info<string_t&>::safe_type::coerce(retval));
					if (pE)
						throw_correct_exception(pE);
					return retval;
				}
			};

			OMEGA_DEFINE_INTERNAL_INTERFACE
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

			OMEGA_QI_MAGIC(Omega,IObject)
			OMEGA_QI_MAGIC(Omega,IException)
			OMEGA_QI_MAGIC(Omega::TypeInfo,ITypeInfo)			
		}
	}
}

// ISafeProxy has no rtti associated with it...
OMEGA_SET_GUIDOF(Omega::System::MetaInfo,ISafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}")

#endif // OOCORE_SAFE_H_INCLUDED_
