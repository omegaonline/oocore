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
			template <typename T> struct type_kind
			{
				static const TypeInfo::Types_t type = TypeInfo::typeUnknown;
			};

			template <typename T> struct type_kind<T*>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeArray | type_kind<T>::type;
			};

			template <typename T> struct type_kind<T&>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeReference | type_kind<T>::type;
			};

			template <typename T> struct type_kind<const T>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeConst | type_kind<T>::type;
			};

			template <> struct type_kind<void>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeVoid;
			};

			template <> struct type_kind<bool_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeBool;
			};

			template <> struct type_kind<byte_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeByte;
			};

			template <> struct type_kind<int16_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeInt16;
			};

			template <> struct type_kind<uint16_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeUInt16;
			};

			template <> struct type_kind<int32_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeInt32;
			};

			template <> struct type_kind<uint32_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeUInt32;
			};

			template <> struct type_kind<int64_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeInt64;
			};

			template <> struct type_kind<uint64_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeUInt64;
			};

			template <> struct type_kind<float4_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeFloat4;
			};

			template <> struct type_kind<float8_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeFloat8;
			};

			template <> struct type_kind<string_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeString;
			};

			template <> struct type_kind<guid_t>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeGuid;
			};

			template <> struct type_kind<IObject*>
			{
				static const TypeInfo::Types_t type = TypeInfo::typeObject;
			};

			struct typeinfo_rtti
			{
				template <bool E, typename I>
				struct has_guid_t
				{
					static const guid_base_t* guid() { return &OMEGA_GUIDOF(I); }
				};

				template <typename I>
				struct has_guid_t<false,I>
				{
					static const guid_base_t* guid() { return 0; }
				};

				struct ParamInfo
				{
					const char*                 pszName;
					TypeInfo::Types_t           type;
					TypeInfo::ParamAttributes_t attribs;
					const char*                 attrib_ref;
					const guid_base_t*          iid;
				};

				struct MethodInfo
				{
					const char*                  pszName;
					TypeInfo::MethodAttributes_t attribs;
					uint32_t                     timeout;
					byte_t                       param_count;
					TypeInfo::Types_t            return_type;
					const ParamInfo* (*pfnGetParamInfo)();
				};

				const MethodInfo* (*pfnGetMethodInfo)();
				uint32_t method_count;
				const guid_base_t* base_type;
			};

			template <typename I>
			class TypeInfo_Holder;

			template <>
			class TypeInfo_Holder<IObject>
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
						{ 0, 0, 0, "", 0 }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* Release_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ 0, 0, 0, "", 0 }
					};
					return pi;
				}
				static const typeinfo_rtti::ParamInfo* QueryInterface_params()
				{
					static const typeinfo_rtti::ParamInfo pi[] =
					{
						{ "iid", TypeInfo::typeGuid | TypeInfo::typeConst | TypeInfo::typeReference, TypeInfo::attrIn, "", 0 },
						{ 0, 0, 0, "", 0 }
					};
					return pi;
				}
				static const typeinfo_rtti::MethodInfo* method_info()
				{
					static const typeinfo_rtti::MethodInfo methods[] =
					{
						{ "AddRef", TypeInfo::Synchronous, 0, 0, TypeInfo::typeVoid, &AddRef_params },
						{ "Release", TypeInfo::Synchronous, 0, 0, TypeInfo::typeVoid, &Release_params },
						{ "QueryInterface", TypeInfo::Synchronous, 0, 1, TypeInfo::typeObject, &QueryInterface_params },
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
