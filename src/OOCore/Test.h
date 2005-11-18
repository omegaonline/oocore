#ifndef OOCORE_TEST_H_INCLUDED_
#define OOCORE_TEST_H_INCLUDED_

#include "./Object.h"
#include "./ProxyStub.h"

namespace OOCore
{
	class Test :
		public OOObj::Object
	{
	public:
		virtual int Array_Test_In(OOObj::uint32_t count, OOObj::uint16_t* pArray) = 0;
		virtual int Array_Test_Out(OOObj::uint32_t* count, OOObj::uint16_t** pArray) = 0;
		virtual int Array_Test_InOut(OOObj::uint32_t* count, OOObj::uint16_t** pArray) = 0;

		DECLARE_IID(OOCore_Export);
	};

	BEGIN_DECLARE_AUTO_PROXY_STUB(Test)
		BEGIN_PROXY_MAP()
			PROXY_ENTRY_2(Array_Test_In,OOObj::uint32_t count,OOObj::uint16_t* pArray)
				PROXY_PARAMS_2(count,array(pArray,0))
			PROXY_ENTRY_2(Array_Test_Out,OOObj::uint32_t* count,OOObj::uint16_t** pArray)
				PROXY_PARAMS_2(count,array(pArray,0))
			PROXY_ENTRY_2(Array_Test_InOut,OOObj::uint32_t* count,OOObj::uint16_t** pArray)
				PROXY_PARAMS_2(count,array(pArray,0,true))
		END_PROXY_MAP()
		
		BEGIN_STUB_MAP()
			STUB_ENTRY_2(Array_Test_In,OOObj::uint32_t,OOObj::uint16_t*)
			STUB_ENTRY_2(Array_Test_Out,OOObj::uint32_t*,OOObj::uint16_t**)
			STUB_ENTRY_2(Array_Test_InOut,OOObj::uint32_t*,OOObj::uint16_t**)
		END_STUB_MAP()
	END_DECLARE_AUTO_PROXY_STUB()

};

#endif // OOCORE_TEST_H_INCLUDED_
