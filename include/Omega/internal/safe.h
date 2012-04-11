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
		namespace Internal
		{
			template <typename T>
			struct std_safe_type
			{
				typedef T type;

				static T coerce(typename optimal_param<T>::type val, ...)
				{
					return val;
				}

				static T clone(typename optimal_param<T>::type val)
				{
					return val;
				}
			};

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
					if (!val)
						OMEGA_THROW("Null pointer passed for reference");

					return *val;
				}
			};

			template <typename T> struct custom_safe_type;
			template <typename T> struct custom_wire_type;

			template <typename T>
			struct custom_safe_type_wrapper
			{
				typedef typename custom_safe_type<T>::impl impl;
				typedef typename impl::safe_type type;

				static typename impl::safe_type_wrapper coerce(typename optimal_param<T>::type val)
				{
					return typename impl::safe_type_wrapper(val);
				}

				template <typename S>
				static typename impl::safe_type_wrapper coerce(typename optimal_param<T>::type val, S count)
				{
					return typename impl::safe_type_wrapper(val,count);
				}

				static typename impl::type_wrapper coerce(type val)
				{
					return typename impl::type_wrapper(val);
				}

				template <typename S>
				static typename impl::type_wrapper coerce(type val, S count)
				{
					return typename impl::type_wrapper(val,count);
				}

				template <typename S>
				static typename impl::type_wrapper coerce(type val, S* count)
				{
					return coerce(val,*count);
				}

				static type clone(typename optimal_param<T>::type val)
				{
					return impl::clone(val);
				}

				static T clone(type val)
				{
					return impl::clone(val);
				}
			};

			template <typename T>
			struct custom_safe_type_const_wrapper
			{
				typedef typename custom_safe_type<T>::impl impl;
				typedef const typename impl::safe_type type;

				static typename impl::safe_type_wrapper coerce(typename optimal_param<const T>::type val)
				{
					return typename impl::safe_type_wrapper(val);
				}

				template <typename S>
				static typename impl::safe_type_wrapper coerce(typename optimal_param<const T>::type val, S count)
				{
					return typename impl::safe_type_wrapper(val,count);
				}

				static typename impl::type_wrapper coerce(type val)
				{
					return typename impl::type_wrapper(val);
				}

				template <typename S>
				static typename impl::type_wrapper coerce(type val, S count)
				{
					return typename impl::type_wrapper(val,count);
				}

				template <typename S>
				static typename impl::type_wrapper coerce(type val, S* count)
				{
					return coerce(val,*count);
				}

				static type clone(typename optimal_param<const T>::type val)
				{
					return impl::clone(val);
				}

				static const T clone(type val)
				{
					return impl::clone(val);
				}
			};

			template <typename T>
			struct custom_safe_type_ref_wrapper
			{
				typedef typename custom_safe_type<T>::impl impl;
				typedef typename impl::safe_type* type;

				struct ref_holder
				{
					operator T&()
					{
						return m_val;
					}

				protected:
					typename impl::type_wrapper m_val;
					type                        m_dest;

					ref_holder(type val) : m_val(*val), m_dest(val)
					{ }
				};

				struct ref_holder_lite : public ref_holder
				{
					ref_holder_lite(type val) : ref_holder(val)
					{}

					~ref_holder_lite()
					{
						this->m_val.update(*this->m_dest);
					}
				};

				struct ref_holder_full : public ref_holder
				{
					ref_holder_full(type val, const guid_base_t* piid) :
							ref_holder(val), m_piid(piid)
					{}

					~ref_holder_full()
					{
						this->m_val.update(*this->m_dest,m_piid);
					}

				private:
					const guid_base_t* m_piid;
				};

				struct ref_holder_safe
				{
					operator type()
					{
						return &m_val;
					}

				protected:
					typename impl::safe_type_wrapper m_val;
					T&                               m_dest;

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
					ref_holder_safe_full(T& val, const guid_t& iid) :
							ref_holder_safe(val), m_iid(iid)
					{}

					~ref_holder_safe_full()
					{
						this->m_val.update(this->m_dest,m_iid);
					}

					ref_holder_safe_full(const ref_holder_safe_full& rhs) : ref_holder_safe(rhs), m_iid(rhs.m_iid)
					{}

				private:
					const guid_t& m_iid;

					ref_holder_safe_full& operator = (const ref_holder_safe_full&);
				};

				static ref_holder_safe_lite coerce(T& val)
				{
					return ref_holder_safe_lite(val);
				}

				static ref_holder_safe_full coerce(T& val, const guid_t& iid)
				{
					return ref_holder_safe_full(val,iid);
				}

				static ref_holder_lite coerce(type val)
				{
					if (!val)
						OMEGA_THROW("Null pointer passed for reference");

					return ref_holder_lite(val);
				}

				static ref_holder_full coerce(type val, const guid_base_t* piid)
				{
					if (!val)
						OMEGA_THROW("Null pointer passed for reference");

					return ref_holder_full(val,piid);
				}
			};

			template <typename T>
			struct custom_safe_type_const_ref_wrapper
			{
				typedef typename custom_safe_type<T>::impl impl;
				typedef const typename impl::safe_type* type;

				struct ref_holder_safe
				{
					ref_holder_safe(typename optimal_param<T>::type val) : m_val(val)
					{}

					operator type()
					{
						return &m_val;
					}

				private:
					typename impl::safe_type_wrapper m_val;
				};

				static ref_holder_safe coerce(typename optimal_param<T>::type val)
				{
					return ref_holder_safe(val);
				}

				static typename impl::type_wrapper coerce(type val)
				{
					if (!val)
						OMEGA_THROW("Null pointer passed for reference");

					return typename impl::type_wrapper(*val);
				}
			};

			// Don't pass pointers by const& - just pass the pointer
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
							m_val(default_value<typename marshal_info<T>::safe_type::type>::value()),
							m_pVals(0),
							m_pOrig(vals),
							m_cbSize(cbSize)
					{
						if (m_pOrig)
						{
							if (cbSize == 1)
								m_val = marshal_info<T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								m_pVals = static_cast<T*>(System::Allocate(cbSize*sizeof(T)));
								for (size_t i=0; i<cbSize; ++i)
									new (m_pVals+i) T(marshal_info<T>::safe_type::coerce(vals[i]));
							}
						}
					}

					~type_wrapper()
					{
						if (m_pOrig)
						{
							if (m_pVals)
							{
								for (size_t i=0; i<m_cbSize; ++i)
								{
									static_cast<T&>(marshal_info<T&>::safe_type::coerce(m_pOrig+i)) = m_pVals[i];
									m_pVals[i].~T();
								}

								System::Free(m_pVals);
							}
							else
								m_val.update(*m_pOrig);
						}
					}

					operator T*()
					{
						if (!m_pOrig)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &static_cast<T&>(m_val);
					}

				private:
					typename marshal_info<T>::safe_type::impl::type_wrapper m_val;
					T*           m_pVals;
					safe_type    m_pOrig;
					const size_t m_cbSize;
				};

				struct safe_type_wrapper
				{
					typedef typename marshal_info<T>::safe_type::type arr_type;

					safe_type_wrapper(T* vals, size_t cbSize) :
							m_val(default_value<T>::value()),
							m_pVals(0),
							m_pOrig(vals),
							m_cbSize(cbSize)
					{
						if (m_pOrig)
						{
							if (cbSize == 1)
								m_val = marshal_info<T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								m_pVals = static_cast<arr_type*>(System::Allocate(cbSize*sizeof(arr_type)));
								for (size_t i=0; i<cbSize; ++i)
									new (m_pVals+i) arr_type(marshal_info<T>::safe_type::coerce(vals[i]));
							}
						}
					}

					~safe_type_wrapper()
					{
						if (m_pOrig)
						{
							if (m_pVals)
							{
								for (size_t i=0; i<m_cbSize; ++i)
								{
									*marshal_info<T&>::safe_type::coerce(m_pOrig[i]) = m_pVals[i];
									m_pVals[i].~arr_type();
								}

								System::Free(m_pVals);
							}
							else
								m_val.update(*m_pOrig);
						}
					}

					operator safe_type()
					{
						if (!m_pOrig)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					typename custom_safe_type<T>::impl::safe_type_wrapper m_val;
					arr_type*    m_pVals;
					T* const     m_pOrig;
					const size_t m_cbSize;
				};
			};

			template <typename T>
			struct array_safe_type<const T>
			{
				typedef typename marshal_info<const T>::safe_type::type* safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type vals, size_t cbSize) :
							m_val(default_value<T>::value()),
							m_pVals(0),
							m_cbSize(!vals ? 0 : cbSize)
					{
						if (vals)
						{
							if (cbSize == 1)
								m_val = marshal_info<const T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								m_pVals = static_cast<T*>(System::Allocate(cbSize*sizeof(T)));
								for (size_t i=0; i<cbSize; ++i)
									new (m_pVals+i) T(marshal_info<const T>::safe_type::coerce(vals[i]));
							}
						}
					}

					~type_wrapper()
					{
						if (m_pVals)
						{
							for (size_t i=0; i<m_cbSize; ++i)
								m_pVals[i].~T();

							System::Free(m_pVals);
						}
					}

					operator const T*()
					{
						if (m_cbSize == 0)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					T            m_val;
					T*           m_pVals;
					const size_t m_cbSize;

					type_wrapper& operator = (const type_wrapper&);
				};

				struct safe_type_wrapper
				{
					typedef typename marshal_info<T>::safe_type::type arr_type;

					safe_type_wrapper(const T* vals, size_t cbSize) :
							m_val(default_value<arr_type>::value()),
							m_pVals(0),
							m_cbSize(!vals ? 0 : cbSize)
					{
						if (vals)
						{
							if (cbSize == 1)
								m_val = marshal_info<const T>::safe_type::coerce(*vals);
							else if (cbSize > 1)
							{
								m_pVals = static_cast<arr_type*>(System::Allocate(cbSize*sizeof(arr_type)));
								for (size_t i=0; i<cbSize; ++i)
									new (m_pVals+i) arr_type(marshal_info<const T>::safe_type::coerce(vals[i]));
							}
						}
					}

					~safe_type_wrapper()
					{
						if (m_pVals)
						{
							for (size_t i=0; i<m_cbSize; ++i)
								m_pVals[i].~arr_type();

							System::Free(m_pVals);
						}
					}

					operator safe_type()
					{
						if (!m_cbSize)
							return 0;
						else if (m_pVals)
							return m_pVals;
						else
							return &m_val;
					}

				private:
					arr_type     m_val;
					arr_type*    m_pVals;
					const size_t m_cbSize;

					safe_type_wrapper& operator = (const safe_type_wrapper&);
				};
			};

			template <typename T>
			struct custom_safe_type<T*>
			{
				typedef array_safe_type<T> impl;
			};

			struct string_t_safe_type
			{
				typedef string_t::handle_t safe_type;

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(string_t_safe_type::create(val,true))
					{ }

					void update(safe_type& dest)
					{
						safe_type old = dest;
						dest = string_t_safe_type::addref(m_val,true);
						string_t_safe_type::release(old);
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
					safe_type_wrapper(const string_t& val) : m_val(string_t_safe_type::addref(val,false))
					{ }

					~safe_type_wrapper()
					{
						string_t_safe_type::release(m_val);
					}

					void update(string_t& dest)
					{
						dest = string_t_safe_type::create(m_val,true);
					}

					operator safe_type()
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

				static safe_type clone(const string_t& s)
				{
					return addref(s,true);
				}

				static string_t clone(safe_type v)
				{
					return create(v,false);
				}

				static string_t create(safe_type v, bool addref)
				{
					return string_t(v,addref);
				}

				static safe_type addref(const string_t& v, bool own)
				{
					// We only need to take ownership if we are passing out of a dll that isn't OOCore
					safe_type h = v.m_handle;
#if !defined(OOCORE_INTERNAL)
					string_t::addref(h,own);
#else
					OMEGA_UNUSED_ARG(own);
					string_t::addref(h,false);
#endif
					return h;
				}

				static void release(safe_type v)
				{
					string_t::release(v);
				}
			};

			template <>
			struct custom_safe_type<string_t>
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
					safe_type_wrapper(bool_t val) : m_val(val ? 1 : 0)
					{}

					void update(bool_t& dest)
					{
						dest = (m_val != 0);
					}

					operator safe_type()
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

				static safe_type clone(bool_t val)
				{
					return (val ? 1 : 0);
				}

				static bool_t clone(safe_type val)
				{
					return (val != 0);
				}
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
					safe_type_wrapper(const guid_t& val) : m_val(val)
					{}

					void update(guid_t& dest)
					{
						dest = m_val;
					}

					operator safe_type()
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

				static safe_type clone(const guid_t& val)
				{
					return val;
				}

				static guid_t clone(const safe_type& val)
				{
					return guid_t(val);
				}
			};

			struct any_t_safe_type
			{
				struct safe_type
				{
					TypeInfo::Type_t type;
					union discrim
					{
						// Stock types
						byte_t           byVal;
						int16_t          i16Val;
						uint16_t         ui16Val;
						int32_t          i32Val;
						uint32_t         ui32Val;
						int64_t          i64Val;
						uint64_t         ui64Val;
						float4_t         fl4Val;
						float8_t         fl8Val;

						// Custom safe types
						custom_safe_type<bool_t>::safe_type bVal;
						string_t_safe_type::safe_type       pstrVal;
						guid_base_t                         gVal;
					} u;
				};

				struct type_wrapper
				{
					type_wrapper(safe_type val) : m_val(any_t_safe_type::create(val,true))
					{ }

					void update(safe_type& dest)
					{
						any_t_safe_type::release(dest);
						dest = any_t_safe_type::addref(m_val);
					}

					operator any_t&()
					{
						return m_val;
					}

				private:
					any_t m_val;
				};
				friend struct type_wrapper;

				struct safe_type_wrapper
				{
					safe_type_wrapper(const any_t& val) : m_val(any_t_safe_type::addref(val))
					{ }

					~safe_type_wrapper()
					{
						any_t_safe_type::release(m_val);
					}

					void update(any_t& dest)
					{
						dest = any_t_safe_type::create(m_val,true);
					}

					operator safe_type()
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

				static safe_type clone(const any_t& s)
				{
					return addref(s);
				}

				static any_t clone(safe_type v)
				{
					return create(v,false);
				}

			private:
				static any_t create(safe_type v, bool addref)
				{
					switch (v.type)
					{
					case TypeInfo::typeBool:
						return static_cast<bool_t>(v.u.bVal != 0);
					case TypeInfo::typeByte:
						return v.u.byVal;
					case TypeInfo::typeInt16:
						return v.u.i16Val;
					case TypeInfo::typeUInt16:
						return v.u.ui16Val;
					case TypeInfo::typeInt32:
						return v.u.i32Val;
					case TypeInfo::typeUInt32:
						return v.u.ui32Val;
					case TypeInfo::typeInt64:
						return v.u.i64Val;
					case TypeInfo::typeUInt64:
						return v.u.ui64Val;
					case TypeInfo::typeFloat4:
						return v.u.fl4Val;
					case TypeInfo::typeFloat8:
						return v.u.fl8Val;
					case TypeInfo::typeString:
						return string_t_safe_type::create(v.u.pstrVal,addref);
					case TypeInfo::typeGuid:
						return guid_t(v.u.gVal);
					case TypeInfo::typeVoid:
					default:
						return any_t();
					}
				}

				static safe_type addref(const any_t& val)
				{
					safe_type ret;
					ret.type = static_cast<TypeInfo::Type_t>(val.m_type);
					switch (val.m_type)
					{
					case TypeInfo::typeBool:
						ret.u.bVal = (val.u.bVal ? 1 : 0);
						break;
					case TypeInfo::typeByte:
						ret.u.byVal = val.u.byVal;
						break;
					case TypeInfo::typeInt16:
						ret.u.i16Val = val.u.i16Val;
						break;
					case TypeInfo::typeUInt16:
						ret.u.ui16Val = val.u.ui16Val;
						break;
					case TypeInfo::typeInt32:
						ret.u.i32Val = val.u.i32Val;
						break;
					case TypeInfo::typeUInt32:
						ret.u.ui32Val = val.u.ui32Val;
						break;
					case TypeInfo::typeInt64:
						ret.u.i64Val = val.u.i64Val;
						break;
					case TypeInfo::typeUInt64:
						ret.u.ui64Val = val.u.ui64Val;
						break;
					case TypeInfo::typeFloat4:
						ret.u.fl4Val = val.u.fl4Val;
						break;
					case TypeInfo::typeFloat8:
						ret.u.fl8Val = val.u.fl8Val;
						break;
					case TypeInfo::typeString:
						ret.u.pstrVal = string_t_safe_type::addref(val.strVal,true);
						break;
					case TypeInfo::typeGuid:
						ret.u.gVal = val.u.gVal;
						break;
					case TypeInfo::typeVoid:
					default:
						break;
					}
					return ret;
				}

				static void release(safe_type val)
				{
					if (val.type == TypeInfo::typeString)
						string_t_safe_type::release(val.u.pstrVal);
				}
			};

			template <>
			struct custom_safe_type<any_t>
			{
				typedef struct any_t_safe_type impl;
			};

			template <typename T> struct is_message_type
			{
				enum { result = 0 };
			};

			// These are the types that can be natively marshalled
			template <> struct is_message_type<bool_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<byte_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<int16_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<uint16_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<int32_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<uint32_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<int64_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<uint64_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<float4_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<float8_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<string_t>
			{
				enum { result = 1 };
			};
			template <> struct is_message_type<guid_t>
			{
				enum { result = 1 };
			};

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

			template <typename T> struct marshal_info<const T>
			{
				typedef typename if_else_t<
					is_c_abi<T>::result,
					std_safe_type<const T>,
					custom_safe_type_const_wrapper<T>
				>::result safe_type;

				typedef typename if_else_t<
					is_message_type<T>::result,
					std_wire_type<const T>,
					custom_wire_type_wrapper<const T>
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
					std_wire_type<T>,
					custom_wire_type_wrapper<T>
				>::result wire_type;
			};

			template <typename T> struct marshal_info<const T&>
			{
				typedef typename if_else_t<
					is_c_abi<T>::result,
					std_safe_type_ref<const T>,
					custom_safe_type_const_ref_wrapper<T>
				>::result safe_type;

				typedef typename if_else_t<
					is_message_type<T>::result,
					std_wire_type<const T>,
					custom_wire_type_wrapper<const T>
				>::result wire_type;
			};

			template <typename T> struct marshal_info<T*>
			{
				typedef typename if_else_t<
					is_c_abi<T>::result,
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
			};

			void throw_correct_exception(const SafeShim* except);
			const SafeShim* return_safe_exception(IException* pE);

			struct IObject_Safe_VTable
			{
				const SafeShim* (OMEGA_CALL* pfnAddRef_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnRelease_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnQueryInterface_Safe)(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid);
				const SafeShim* (OMEGA_CALL* pfnPin_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnUnpin_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnCreateWireStub_Safe)(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_base_t* piid, const SafeShim** retval);
				const SafeShim* (OMEGA_CALL* pfnGetWireProxy_Safe)(const SafeShim* shim, const SafeShim** retval);
			};

			inline const SafeShim* addref_safe(const SafeShim* shim)
			{
				if (shim)
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnAddRef_Safe(shim);
					throw_correct_exception(except);
				}
				return shim;
			}

			inline void release_safe(const SafeShim* shim)
			{
				if (shim)
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnRelease_Safe(shim);
					throw_correct_exception(except);
				}
			}

			IObject* create_safe_proxy(const SafeShim* shim, const guid_t& iid);

			template <typename I>
			I* create_safe_proxy(const SafeShim* shim)
			{
				return static_cast<I*>(create_safe_proxy(shim,OMEGA_GUIDOF(I)));
			}

			const SafeShim* create_safe_stub(IObject* pObject, const guid_t& iid);

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
						m_pI = create_safe_proxy<I>(pS);
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

					void update(safe_type& pS, const guid_base_t* piid = 0)
					{
						if (pS)
							release_safe(pS);

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
							release_safe(m_pS);
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

						pI = create_safe_proxy<I>(m_pS);
					}

					void update(I*& pI, const guid_t& iid)
					{
						if (pI)
							pI->Release();

						pI = static_cast<I*>(create_safe_proxy(m_pS,iid));
					}

				private:
					safe_type m_pS;
				};

				static safe_type clone(I* pI)
				{
					return create_safe_stub(pI,OMEGA_GUIDOF(I));
				}

				static I* clone(safe_type shim)
				{
					return create_safe_proxy<I>(shim);
				}
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
		}
	}
}

#define OMEGA_CREATE_INTERNAL(e)  (Omega::System::Internal::auto_iface_ptr<Omega::IInternalException>(Omega::IInternalException::Create(e,__FILE__,__LINE__,OMEGA_FUNCNAME)))

#endif // OOCORE_SAFE_H_INCLUDED_
