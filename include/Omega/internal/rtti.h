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

			template <typename T> struct type_kind<std::vector<T> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierSTLVector, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<std::deque<T> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierSTLDeque, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<std::list<T> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierSTLList, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<std::set<T> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierSTLSet, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T> struct type_kind<std::multiset<T> >
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::modifierSTLMultiset, type_kind<T>::type() };
					return &t;
				}
			};

			template <typename T1, typename T2> struct type_kind<std::map<T1,T2> >
			{
				static const type_holder* type()
				{
					static const type_holder t[2] = 
					{
						{ TypeInfo::modifierSTLMap, type_kind<T1>::type() },
						{ TypeInfo::modifierSTLMap, type_kind<T2>::type() }
					};
					return t;
				}
			};

			template <typename T1, typename T2> struct type_kind<std::multimap<T1,T2> >
			{
				static const type_holder* type()
				{
					static const type_holder t[2] = 
					{
						{ TypeInfo::modifierSTLMultimap, type_kind<T1>::type() },
						{ TypeInfo::modifierSTLMultimap, type_kind<T2>::type() }
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

			template <> struct type_kind<IObject*>
			{
				static const type_holder* type()
				{
					static const type_holder t = { TypeInfo::typeObjectPtr, (const type_holder*)(&OMEGA_GUIDOF(IObject)) };
					return &t;
				}
			};

			struct typeinfo_rtti
			{
				struct ParamInfo
				{
					const wchar_t*              pszName;
					const type_holder*          type;
					TypeInfo::ParamAttributes_t attribs;
					const wchar_t*              attrib_ref;
				};

				struct MethodInfo
				{
					const wchar_t*               pszName;
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
						{ 0, 0, 0, L"" }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* Release_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ 0, 0, 0, L"" }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* QueryInterface_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ L"iid", type_kind<const guid_t&>::type(), TypeInfo::attrIn, L"" },
						{ 0, 0, 0, L"" }
					};
					return pi;
				}
				static const typeinfo_rtti::MethodInfo* method_info()
				{
					static const typeinfo_rtti::MethodInfo methods[] =
					{
						{ L"AddRef", TypeInfo::Synchronous, 0, 0, type_kind<void>::type(), &AddRef_params },
						{ L"Release", TypeInfo::Synchronous, 0, 0, type_kind<void>::type(), &Release_params },
						{ L"QueryInterface", TypeInfo::Synchronous, 0, 1, type_kind<IObject*>::type(), &QueryInterface_params },
						{ 0, 0, 0, 0, 0 }
					};
					return methods;
				}
			};

			void register_typeinfo(const guid_t& iid, const wchar_t* pszName, const typeinfo_rtti* type_info);
			void unregister_typeinfo(const guid_t& iid, const typeinfo_rtti* type_info);
		}
	}
}

#endif // OOCORE_RTTI_H_INCLUDED_
