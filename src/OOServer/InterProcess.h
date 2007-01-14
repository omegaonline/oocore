#ifndef OOSERVER_INTERPROCESS_H_INCLUDED_
#define OOSERVER_INTERPROCESS_H_INCLUDED_

namespace OOServer
{
	class IInterProcess : public Omega::IObject
	{
	public:
		virtual Omega::Registry::IRegistryKey* GetRegistryKey() = 0;
		virtual Omega::Activation::IRunningObjectTable* GetROT() = 0;
	};
	OMEGA_DECLARE_IID(IInterProcess);

	class IStaticInterProcess : public Omega::IObject
	{
	public:
		virtual IInterProcess* Init() = 0;
	};
	OMEGA_DECLARE_IID(IStaticInterProcess);
}

OMEGA_EXPORT_INTERFACE
(
	OOServer, IInterProcess,
	0x299da443, 0xd8de, 0x45f0, 0xb7, 0x21, 0x74, 0x90, 0xac, 0x8e, 0x60, 0x5,

	OMEGA_METHOD(Omega::Registry::IRegistryKey*,GetRegistryKey,0,())
	OMEGA_METHOD(Omega::Activation::IRunningObjectTable*,GetROT,0,())
)

OMEGA_EXPORT_INTERFACE
(
	OOServer, IStaticInterProcess,
	0x355c529b, 0x579a, 0x411d, 0xb1, 0x6c, 0x31, 0x23, 0xbc, 0x85, 0x5f, 0xbf,

	OMEGA_METHOD(OOServer::IInterProcess*,Init,0,())
)

OMEGA_DEFINE_OID(OOServer, OID_InterProcess, 0x525e8e60, 0xdb0f, 0x42d7, 0x93, 0x8b, 0x86, 0x7e, 0x0, 0x8b, 0xe3, 0x6);

#endif // OOSERVER_INTERPROCESS_H_INCLUDED_
