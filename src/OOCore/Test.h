#ifndef OOCORE_TEST_H_INCLUDED_
#define OOCORE_TEST_H_INCLUDED_

#include "./Object.h"
#include "./ProxyStub.h"
#include "./Constructor.h"

namespace OOCore
{
	// TODO: MOVE THIS ELSEWHERE!
	class Server : public OOObject::Object
	{
	public:
		virtual OOObject::int32_t Stop(OOObject::bool_t force, OOObject::uint16_t* remaining) = 0;
		virtual OOObject::int32_t StopPending(OOObject::bool_t* pending) = 0;
		virtual OOObject::int32_t StayAlive() = 0;

		DECLARE_IID(OOCore_Export);
	};

	BEGIN_AUTO_PROXY_STUB(Server)
		METHOD(Stop,2,((in),OOObject::bool_t,force,(out),OOObject::uint16_t*,remaining))
		METHOD(StopPending,1,((out),OOObject::bool_t*,pending))
		METHOD(StayAlive,0,())
	END_AUTO_PROXY_STUB()

	class Test : public OOObject::Object
	{
	public:
		virtual int Array_Test_In(OOObject::uint32_t count, OOObject::uint16_t* pArray) = 0;
		virtual int Array_Test_Out(OOObject::uint32_t* count, OOObject::uint16_t** pArray) = 0;
		virtual int Array_Test_InOut(OOObject::uint32_t* count, OOObject::uint16_t** pArray) = 0;

		virtual int Object_Test_In(const OOObject::guid_t& iid, OOObject::Object* pObj) = 0;
		virtual int Object_Test_Out(const OOObject::guid_t& iid, OOObject::Object** pObj) = 0;
		
		DECLARE_IID(OOCore_Export);
	};

	BEGIN_AUTO_PROXY_STUB(Test)
		METHOD(Array_Test_Out,2,((out),OOObject::uint32_t*,count,(out)(size_is(count)),OOObject::uint16_t**,pArray))
		METHOD(Array_Test_In,2,((in),OOObject::uint32_t,count,(in)(size_is(count)),OOObject::uint16_t*,pArray))	
		METHOD(Array_Test_InOut,2,((in)(out),OOObject::uint32_t*,count,(in)(out)(size_is(count)),OOObject::uint16_t**,pArray))

		METHOD(Object_Test_Out,2,((in),const OOObject::guid_t&,iid,(out)(iid_is(iid)),OOObject::Object**,pObj))
		METHOD(Object_Test_In,2,((in),const OOObject::guid_t&,iid,(in)(iid_is(iid)),OOObject::Object*,pObj))
	END_AUTO_PROXY_STUB()

	/*class Test_CONSTRUCTOR : public OOCore::Constructor_Impl<Test>
	{
	private:
		friend class OOCore::Impl::creator_t;

		int create_i(int method, const OOObject::guid_t& iid, OOObject::Object** ppVal, OOCore::Impl::InputStream_Wrapper& in, OOCore::Impl::OutputStream_Wrapper& out)
		{
			return OOCore::Impl::creator_t::create_i(method,this,iid,ppVal,in,out);
		}

	// Per method stuff
	public:
		static int create_object(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			OOCore::Impl::constructor_t cons = constructor(0);
			int ret = cons.send_and_recv(iid,ppVal);
			return ret;
		}
	private:
		int create_i(boost::mpl::int_<0>&,const OOObject::guid_t& iid, OOObject::Object** ppVal, OOCore::Impl::InputStream_Wrapper& in, OOCore::Impl::OutputStream_Wrapper& out)
		{
			// Unpack

			CLASS* pClass;
			ACE_NEW_RETURN(pClass,CLASS,-1);
			int ret_code = pClass->QueryInterface(iid,ppVal);
			if (ret_code==0)
			{
				// return ?!?!
			}
			
			return ret_code

			return 0;
		}

	public:
		static int create_object(const OOObject::guid_t& iid, OOObject::Object** ppVal, int in, int* out)
		{
            OOCore::Impl::constructor_t cons = constructor(0);
			cons << in;
			int ret = cons.send_and_recv(iid,ppVal);
			if (ret==0)
			{
				cons >> out;
			}
			return ret;
		}
	};*/
};

#endif // OOCORE_TEST_H_INCLUDED_