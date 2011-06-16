class Aggregator :
		public Omega::TestSuite::ISimpleTest2
{
public:
	Aggregator() : m_pInner(0)
	{
		AddRef();
	}

	void SetInner(Omega::IObject* pInner)
	{
		m_pInner = pInner;
	}

	void AddRef()
	{
		m_refcount.AddRef();
	}

	void Release()
	{
		assert(!m_refcount.IsZero());

		if (m_refcount.Release())
			delete this;
	}

	Omega::IObject* QueryInterface(const Omega::guid_t& iid)
	{
		if (iid == OMEGA_GUIDOF(Omega::IObject) ||
				iid == OMEGA_GUIDOF(Omega::TestSuite::ISimpleTest2))
		{
			AddRef();
			return this;
		}

		if (m_pInner)
		{
			Omega::IObject* pObj = m_pInner->QueryInterface(iid);
			if (pObj)
				return pObj;
		}

		return 0;
	}

	Omega::string_t WhereAmI()
	{
		return Omega::string_t(L"Outer");
	}

private:
	virtual ~Aggregator()
	{
		if (m_pInner)
			m_pInner->Release();
	}

	Omega::Threading::AtomicRefCount m_refcount;
	Omega::IObject*   m_pInner;
};
