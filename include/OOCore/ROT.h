#ifndef OOCORE_ROT_H_INCLUDED_
#define OOCORE_ROT_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega 
{
namespace Activation
{
	interface IRunningObjectTable : public IObject
	{
		virtual void GetRegisteredObject(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;

		static IRunningObjectTable* GetRunningObjectTable();
	};
	OMEGA_DECLARE_IID(IRunningObjectTable);
}
}

OMEGA_EXPORT_INTERFACE
(
	Omega::Activation, IRunningObjectTable, 
	0x8e26d026, 0x9988, 0x4f69, 0x93, 0xc3, 0xc4, 0x72, 0x43, 0x4f, 0x9d, 0xde,

	// Methods
	OMEGA_METHOD_VOID(GetRegisteredObject,3,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject))
)

#endif // OOCORE_ROT_H_INCLUDED_
