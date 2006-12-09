#ifndef OOCORE_STDAPARTMENT_H_INCLUDED_
#define OOCORE_STDAPARTMENT_H_INCLUDED_

class StdApartment :
	public OTL::ObjectBase,
	public Omega::Activation::IApartment
{
public:
	StdApartment();
	~StdApartment();

	BEGIN_INTERFACE_MAP(StdApartment)
		INTERFACE_ENTRY(Omega::Activation::IApartment)
	END_INTERFACE_MAP()

private:
	struct Request : public ACE_Method_Request
	{
		Request(REQUEST_FN request_fn, void* request_fn_args) :
			m_request_fn(request_fn),
			m_request_fn_args(request_fn_args)
		{ }

		int call()
		{
			int res = 0;
			try
			{
				m_request_fn(m_request_fn_args);
			}
			catch (...)
			{
				res = -1;
			}
			delete this;
			return res;
		}

		REQUEST_FN m_request_fn;
		void* m_request_fn_args;
	};

// IApartment members
public:
	Omega::bool_t PumpRequests(Omega::uint32_t* timeout, PUMP_CONDITION_FN cond_fn = 0, void* cond_fn_args = 0);
	Omega::bool_t PostRequest(REQUEST_FN request_fn, void* request_fn_args, Omega::uint32_t wait = 0);
};

#endif // OOCORE_STDAPARTMENT_H_INCLUDED_