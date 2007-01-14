#ifndef OOSERVER_INTERPROCESS_H_INCLUDED_
#define OOSERVER_INTERPROCESS_H_INCLUDED_

namespace OOServer
{
	class IInterProcess : public Omega::IObject
	{
	public:
		virtual void Init(Omega::Registry::IRegistryKey*& pKey, Omega::Activation::IRunningObjectTable*& pROT) = 0;
	};
	OMEGA_DECLARE_IID(IInterProcess);
}

OMEGA_EXPORT_INTERFACE
(
	OOServer, IInterProcess,
	0x355c529b, 0x579a, 0x411d, 0xb1, 0x6c, 0x31, 0x23, 0xbc, 0x85, 0x5f, 0xbf,

	OMEGA_METHOD_VOID(Init,2,((out),Omega::Registry::IRegistryKey*&,pKey,(out),Omega::Activation::IRunningObjectTable*&,pROT))
)

OMEGA_DEFINE_OID(OOServer, OID_InterProcess, 0x525e8e60, 0xdb0f, 0x42d7, 0x93, 0x8b, 0x86, 0x7e, 0x0, 0x8b, 0xe3, 0x6);

#endif // OOSERVER_INTERPROCESS_H_INCLUDED_
