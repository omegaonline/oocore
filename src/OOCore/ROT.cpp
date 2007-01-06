#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

class RunningObjectTableImpl :
	public ObjectBase,
	public Activation::IRunningObjectTable
{
public:
	void GetRegisteredObject(const guid_t& oid, const guid_t& iid, IObject** ppObject);

	BEGIN_INTERFACE_MAP(RunningObjectTableImpl)
		INTERFACE_ENTRY(Activation::IRunningObjectTable)
	END_INTERFACE_MAP()
};

// RunningObjectTable
Activation::IRunningObjectTable* Activation::IRunningObjectTable::GetRunningObjectTable()
{
	return ObjectImpl<RunningObjectTableImpl>::CreateObject();
}

void RunningObjectTableImpl::GetRegisteredObject(const guid_t&, const guid_t&, IObject**)
{
	void* TODO;
}
