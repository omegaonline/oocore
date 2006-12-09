#ifndef OOCORE_STDTRANSPORT_H_INCLUDED_
#define OOCORE_STDTRANSPORT_H_INCLUDED_

namespace OOCore
{
class StdTransport :
	public ObjectBase,
	public AutoObjectFactory<StdTransport,&OID_StdTransport>,
	public Omega::Remoting::Channels::ITransportHelper,
	public Omega::Remoting::Channels::IInboundChannelSink
{
public:
	StdTransport();
	virtual ~StdTransport();

	BEGIN_INTERFACE_MAP(StdObjectManager)
		
		INTERFACE_ENTRY(Omega::Remoting::Channels::ITransportHelper)
	END_INTERFACE_MAP()


}

#endif // OOCORE_STDTRANSPORT_H_INCLUDED_