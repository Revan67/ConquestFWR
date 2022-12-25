#ifndef PEN_H
#define PEN_H

// Note: CPen and CBrush will automatically handle de-selection and destruction

template<typename HType> struct GDIObject
{
	HType	handle;
	HDC		dc;		// has object been selected?
	HType	old;

	bool is_valid (void)
	{
		return handle != 0;
	}
	bool is_active (void)
	{
		//assert(SelectObject(dc,handle) == handle);
		return dc != 0;
	}

	operator HType (void)
	{
		return handle;
	}

	GDIObject (void)
	{
		handle = 0;
		dc = 0;
		old = 0;
	}

	HType select (HDC _dc)
	{
		assert(handle);
		assert(dc == 0);
		dc = _dc;
		old = (HType)SelectObject(dc,handle);
		return old;
	}

	void deselect (void)
	{
		if (dc)
		{
			SelectObject(dc,old);
			dc = 0;
		}
	}

	void destroy (void)
	{
		if (handle)
		{
			deselect();
			DeleteObject(handle);
			handle = 0;
		}
	}

	~GDIObject (void)
	{
		destroy();
	}
};

struct CPen : GDIObject<HPEN>
{
/*
	HPEN pen;
	HDC dc;		// has object been selected?
	HPEN old;

	operator HPEN (void)
	{
		return pen;
	}

	CPen (void)
	{
		pen = 0;
		dc = 0;
		old = 0;
	}

	void destroy (void)
	{
		if (dc)
		{
			dc->SelectObject(old);
		}
		if (pen)
		{
			DeleteObject(pen);
			pen = 0;
		}
	}

	~CPen (void)
	{
		destroy();
	}
*/
	bool create (int r, int g, int b)
	{
		handle = CreatePen(PS_SOLID,1,RGB(r,g,b));
		return is_valid();
	}
};

struct CBrush : GDIObject<HBRUSH>
{
/*
	HBRUSH handle;

	operator HBRUSH (void)
	{
		return handle;
	}

	CBrush (void)
	{
		handle = 0;
	}

	void destroy (void)
	{
		if (handle)
		{
			DeleteObject(handle);
			handle = 0;
		}
	}

	~CBrush (void)
	{
		destroy();
	}

	HBRUSH select (HDC dc)
	{
		return (HBRUSH) SelectObject(dc,handle);
	}
*/
	bool create (int r, int g, int b)
	{
		handle = CreateSolidBrush(RGB(r,g,b));
		return handle != 0;
	}
};


#endif //PEN_H
