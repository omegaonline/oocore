#include <OOCore/OOCore.h>

namespace Test
{
	class DllTest : public Omega::IObject
	{
	public:
		virtual Omega::string_t Hello() = 0;
	};

	OMEGA_DECLARE_IID(DllTest);
}

OMEGA_EXPORT_INTERFACE
(
	Test, DllTest, 
	0xe295b51f, 0x13b8, 0x4be3, 0x91, 0x9d, 0x39, 0x4c, 0xa3, 0xdb, 0x38, 0x31,

	// Methods
	OMEGA_METHOD(Omega::string_t,Hello,0,())
)

OMEGA_DEFINE_OID(Test, OID_TestDll, 0xe7d34c3d, 0x659c, 0x4ad2, 0xa6, 0xe3, 0x8a, 0x3a, 0xfd, 0x76, 0x49, 0xb);
