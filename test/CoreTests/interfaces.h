#ifndef TEST_INTERFACES_INCLUDED
#define TEST_INTERFACES_INCLUDED

namespace Test
{
	interface Iface : public Omega::IObject
	{
		virtual Omega::string_t Hello() = 0;
	};
}

OMEGA_DEFINE_INTERFACE
(
	Test, Iface, "{8488359E-C953-4e99-B7E5-ECA150C92F48}",

	// Methods
	OMEGA_METHOD(Omega::string_t,Hello,0,())
)

#endif // TEST_INTERFACES_INCLUDED
