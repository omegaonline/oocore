#ifndef OOCORE_ENUM_H_INCLUDED_
#define OOCORE_ENUM_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega
{
	interface IEnumString : public IObject
	{
		virtual uint32_t Next(uint32_t count, string_t* parrVals) = 0;
		virtual void Skip(uint32_t count) = 0;
		virtual void Reset() = 0;
		virtual IEnumString* Clone() = 0;
	};
	OMEGA_DECLARE_IID(IEnumString);
};

OMEGA_EXPORT_INTERFACE
(
	Omega, IEnumString,
	0x154dd0d9, 0xc452, 0x4847, 0xb4, 0xf9, 0xda, 0x64, 0xc0, 0x22, 0xb2, 0x43,

	// Methods
	OMEGA_METHOD(Omega::uint32_t,Next,2,((in),Omega::uint32_t,count,(out)(size_is(count)),Omega::string_t*,parrVals))
	OMEGA_METHOD_VOID(Skip,1,((in),uint32_t,count))
	OMEGA_METHOD_VOID(Reset,0,())
	OMEGA_METHOD(IEnumString*,Clone,0,())
)

#endif // OOCORE_ENUM_H_INCLUDED_
