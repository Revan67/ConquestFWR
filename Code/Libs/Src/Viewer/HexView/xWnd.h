#ifndef XWND_H
#define XWND_H

//---------------------------------------------------------------------------
// xWnd
//---------------------------------------------------------------------------

struct xWnd
{
	HWND hWnd;

	xWnd (void)
	{
		hWnd = 0;
	}

	virtual void Init (HWND h)
	{
		hWnd = h;
	}

	HWND GetParent (void)
	{
		HWND p = ::GetParent(hWnd);
		return p;
	}

	void GetClientRect (RECT *r)
	{
		::GetClientRect(hWnd,r); 
	}

	void InvalidateRect (RECT *r=0, int bErase=0)
	{
		::InvalidateRect(hWnd,r,bErase);
	}
	void ValidateRect (RECT *r)
	{
		::ValidateRect(hWnd,r);
	}

	virtual void OnScroll (void)
	{
	}
	virtual void OnVScroll (UINT wParam, LONG lParam)
	{
		OnScroll();
	}
	virtual void OnHScroll (UINT wParam, LONG lParam)
	{
		OnScroll();
	}

	virtual int OnMove (int x, int y)
	{
		return 0;
	}

	virtual int OnSize (int w, int h)
	{
		return 0;
	}

	virtual void OnDraw (CDC *dc)
	{
	}

	virtual void CmdUI (CCmdUI *cmd)
	{
	}

	void OnPaint (void)
	{
		//xDC xdc(hwnd);
		CDC dc;
		dc.Attach( GetDC(hWnd) );
		OnDraw(&dc);
		dc.Detach();
	}
};

//---------------------------------------------------------------------------
// xScrollBar
//---------------------------------------------------------------------------

struct xScrollBar : xWnd
{
	int bar;	// SB_VERT, SB_HORZ, SB_CTL

	int line;
	int page;

	void init (HWND h, int _bar=SB_CTL)
	{
		xWnd::Init(h);
		bar = _bar;
	}

	xScrollBar (void)
	{
		bar = SB_CTL;
		line = 1;
		page = 4;
	}

	void SetScrollRange (int min, int max)
	{
		::SetScrollRange(hWnd,bar,min,max,TRUE);
	}

	int GetMax (void)
	{
		int min,max;
		::GetScrollRange(hWnd,bar,&min,&max);
		return max;
	}

	void SetScrollSizes (int sizeTotal, int sizePage=4, int sizeLine=1)
	{
		SCROLLINFO info;

		info.cbSize = sizeof(info);
		info.fMask = SIF_RANGE|SIF_PAGE;

		info.nMin = 0;
		info.nMax = sizeTotal-1;
		info.nPage = sizePage;

		SetScrollInfo(hWnd,bar,&info,TRUE);

		line = sizeLine;
		page = sizePage;
	}

	int GetScrollPos (void)
	{
		return ::GetScrollPos(hWnd,bar);
	}
	int SetScrollPos (int pos)
	{
		::SetScrollPos(hWnd,bar,pos,TRUE);
		return ::GetScrollPos(hWnd,bar);
	}

	void OnVScroll (UINT wParam, LONG lParam)
	// WndProc message = WM_VSCROLL
	{
		int id = LOWORD(wParam);
		int p = HIWORD(wParam);
		HWND h = (HWND)lParam;

		int pos = GetScrollPos();

		switch (id)
		{
			case SB_LINEUP:
				pos -= line;
			break;

			case SB_LINEDOWN:
				pos += line;
			break;

			case SB_PAGEUP:
				pos -= page;
			break;

			case SB_PAGEDOWN:
				pos += page;
			break;

			case SB_THUMBPOSITION:		// thumb release
				pos = p;
			break;

			case SB_THUMBTRACK:			// while dragging
			break;

			case SB_TOP:				// begin
				pos = 0;
			break;

			case SB_BOTTOM:				// end
				pos = GetMax();
			break;

			case SB_ENDSCROLL:			// SB_ list terminator
			break;
		}

		SetScrollPos(pos);
	}
};


//---------------------------------------------------------------------------
// xDC
//---------------------------------------------------------------------------

struct xDC
{
	HWND hWnd;
	HDC dc;
	RECT clip;

	xDC (HWND h)
	{
		hWnd = h;
		dc = GetDC(hWnd);
	}

	int GetClipBox (RECT *rect)
	{
		int ok = ::GetClipBox(dc,&clip);
		*rect = clip;
		return (ok);
	}

	void MoveTo (int x, int y)
	{
		::MoveToEx(dc, x,y, NULL);
	}

	void LineTo (int x, int y)
	{
		::LineTo(dc, x,y);
	}

	void TextOut (int x, int y, const char *string)
	{
		::TextOut(dc, x,y, string, strlen(string)); 
	}

	void FillSolidRect (RECT *rect, int color)
	{
		HBRUSH brush = CreateSolidBrush(color);
		FillRect(dc, rect, brush);
		DeleteObject(brush);
	}

	void SetTextAlign (int ta)
	{
		::SetTextAlign(dc,ta);
	}

	void SetBkMode (int mode)
	{
		::SetBkMode(dc,mode);
	}

	void GetTextMetrics (TEXTMETRIC *tm)
	{
		::GetTextMetrics(dc,tm);
	}

	CFont *SelectObject (CFont *font)
	{
		HGDIOBJ hOldObj = ::SelectObject(dc,font->GetSafeHandle());
		return (CFont*)CGdiObject::FromHandle(hOldObj);
	}
};

//---------------------------------------------------------------------------

struct xFont
{
	CFont font;
	CFont *old_font;

	xFont (void)
	{
		old_font = 0;
	}

	void set_font (int h, int w, int bold, const char *type)
	{
		font.CreateFont(
			h,w,				// int nHeight, int nWidth,
			0,					// int nEscapement,
			0,					// int nOrientation,
			bold,				// int nWeight,
			FALSE,				// BYTE bItalic,
			FALSE,				// BYTE bUnderline,
			0,					// BYTE cStrikeOut,
			ANSI_CHARSET,		// BYTE nCharSet,
			OUT_DEFAULT_PRECIS,	// BYTE nOutPrecision,
			CLIP_DEFAULT_PRECIS,// BYTE nClipPrecision,
			DEFAULT_QUALITY,	// BYTE nQuality,
			FF_DONTCARE|FIXED_PITCH,// BYTE nPitchAndFamily,
			type				// LPCTSTR lpszFacename
		);
	}

	void use (CDC *dc)
	{
		CFont *prev = dc->SelectObject(&font);

//		get_metrics();

		if (old_font == NULL)
			old_font = prev;
	}

	void get_dimensions (int *w, int *h)
	{
		LOGFONT info;
		if (font.GetLogFont(&info))
		{
			*w = info.lfWidth;
			*h = info.lfHeight;
		}
	}
};

//---------------------------------------------------------------------------

#endif // XWND_H
