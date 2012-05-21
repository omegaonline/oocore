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

#ifndef OOCORE_RTTI_H_INCLUDED_
#define OOCORE_RTTI_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		namespace Internal
		{
			struct type_holder
			{
				TypeInfo::Type_t   type;
				const type_holder* next;
			};

			template <typename T> struct type_kind;

			template <typename T> struct type_kind<T*>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierPointer, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<T&>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierReference, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<const T>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierConst, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T, typename A> struct type_kind<std::vector<T,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeSTLVector, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T, typename A> struct type_kind<std::deque<T,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeSTLDeque, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T, typename A> struct type_kind<std::list<T,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeSTLList, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T, typename P, typename A> struct type_kind<std::set<T,P,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeSTLSet, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T, typename P, typename A> struct type_kind<std::multiset<T,P,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeSTLMultiset, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename K, typename V, typename P, typename A> struct type_kind<std::map<K,V,P,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t[2] =
					{
						{ TypeInfo::typeSTLMap, type_kind<K>::type() },
						{ TypeInfo::typeSTLMap, type_kind<V>::type() }
					};
					return t;
				}
			};

			template <typename K, typename V, typename P, typename A> struct type_kind<std::multimap<K,V,P,A> >
			{
				static const type_holder* type()
				{
					static const type_holder t[2] =
					{
						{ TypeInfo::typeSTLMultimap, type_kind<K>::type() },
						{ TypeInfo::typeSTLMultimap, type_kind<V>::type() }
					};
					return t;
				}
			};

			template <> struct type_kind<void>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeVoid, 0 };
					return &t;
				}
			};

			template <> struct type_kind<bool_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeBool, 0 };
					return &t;
				}
			};

			template <> struct type_kind<byte_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeByte, 0 };
					return &t;
				}
			};

			template <> struct type_kind<int16_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeInt16, 0 };
					return &t;
				}
			};

			template <> struct type_kind<uint16_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeUInt16, 0 };
					return &t;
				}
			};

			template <> struct type_kind<int32_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeInt32, 0 };
					return &t;
				}
			};

			template <> struct type_kind<uint32_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeUInt32, 0 };
					return &t;
				}
			};

			template <> struct type_kind<int64_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeInt64, 0 };
					return &t;
				}
			};

			template <> struct type_kind<uint64_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeUInt64, 0 };
					return &t;
				}
			};

			template <> struct type_kind<float4_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeFloat4, 0 };
					return &t;
				}
			};

			template <> struct type_kind<float8_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeFloat8, 0 };
					return &t;
				}
			};

			template <> struct type_kind<string_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeString, 0 };
					return &t;
				}
			};

			template <> struct type_kind<guid_t>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeGuid, 0 };
					return &t;
				}
			};

			template <> struct type_kind<IObject>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeObject, (const type_holder*)(&OMEGA_GUIDOF(IObject)) };
					return &t;
				}
			};

			struct typeinfo_rtti
			{
				struct ParamInfo
				{
					const char*                 pszName;
					const type_holder*          type;
					TypeInfo::ParamAttributes_t attribs;
					const char*                 attrib_ref;
				};

				struct MethodInfo
				{
					const char*                  pszName;
					TypeInfo::MethodAttributes_t attribs;
					uint32_t                     timeout;
					byte_t                       param_count;
					const type_holder*           return_type;
					const ParamInfo* (*pfnGetParamInfo)();
				};

				const MethodInfo* (*pfnGetMethodInfo)();
				uint32_t method_count;
				const guid_base_t* base_type;
			};

			template <typename I>
			class typeinfo_holder;

			template <>
			class typeinfo_holder<IObject>
			{
			public:
				static const typeinfo_rtti* get_type_info()
				{
					static const typeinfo_rtti ti = { &method_info, 3, 0 };
					return &ti;
				};

				static const uint32_t method_count = 3;

			private:
				static const typeinfo_rtti::ParamInfo* AddRef_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ 0, 0, 0, "" }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* Release_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ 0, 0, 0, "" }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* QueryInterface_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ "iid", type_kind<const guid_t&>::type(), TypeInfo::attrIn, "" },
						{ 0, 0, 0, "" }
					};
					return pi;
				}
				static const typeinfo_rtti::MethodInfo* method_info()
				{
					static const typeinfo_rtti::MethodInfo methods[] =
					{
						{ "AddRef", TypeInfo::Synchronous, 0, 0, type_kind<void>::type(), &AddRef_params },
						{ "Release", TypeInfo::Synchronous, 0, 0, type_kind<void>::type(), &Release_params },
						{ "QueryInterface", TypeInfo::Synchronous, 0, 1, type_kind<IObject*>::type(), &QueryInterface_params },
						{ 0, 0, 0, 0, 0 }
					};
					return methods;
				}
			};

			void register_typeinfo(const void* key, const guid_t& iid, const char* pszName, const typeinfo_rtti* type_info);
			void unregister_typeinfo(const void* key, const guid_t& iid);
		}
	}
}

#endif // OOCORE_RTTI_H_INCLUDED_
