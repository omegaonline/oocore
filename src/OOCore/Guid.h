//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#pragma once

#include <ace/UUID.h>
#include <ace/CDR_Stream.h>

#include "./OOCore_export.h"

namespace OOObj
{
	class OOCore_Export GUID 
	{
		friend ACE_CDR::Boolean OOCore_Export operator >>(ACE_InputCDR& input, OOObj::GUID& guid);
		friend ACE_CDR::Boolean OOCore_Export operator <<(ACE_OutputCDR& output, const OOObj::GUID& guid);

	public:
		GUID();
		GUID(const ACE_CString& uuidString);
		GUID(const ACE_Utils::UUID& uuid);

		bool operator ==(const GUID& rhs) const;
		bool operator !=(const GUID& rhs) const;
		bool operator <(const GUID& rhs) const;

		ACE_TString to_string() const;

		static const GUID GUID_NIL;

	private:
		ACE_UINT32 Data1;
		ACE_UINT16 Data2;
		ACE_UINT16 Data3;
		ACE_UINT8  Data4[8];

		void init_i(ACE_Utils::UUID& guid);
	};
};