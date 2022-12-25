#ifndef WINDOW_H
#define WINDOW_H

#include "TString.h"

void InitAppControls (void);

extern HINSTANCE AppInstance;

//---------------------------------------------------------------------------
// CWnd
//---------------------------------------------------------------------------

#define Window CWnd

struct CWnd
{
	Window *next;

	TString<64> class_name;

	HWND hWnd;

	HACCEL hAccTable;

	bool dead;
	bool active;

	static Window *Head;
	static Window *FromHandle (HWND h);
	static void Register (Window *w);
	static void UnRegister (Window *w);

	void init (void)
	{
		class_name = "CWnd";

		next = 0;
		hWnd = 0;

		hAccTable = 0;

		Register(this);

		dead = 0;
		active = 0;
	}

	CWnd (void)
	{
		init();
	}

	bool SetMenu (int idr_menu)
	{
		return ::SetMenu(hWnd,(HMENU)idr_menu) != 0;
	}

	void SetAccelerator (int idr_accel)
	{
		if (hAccTable)
		{
			//delete
		}
		hAccTable = LoadAccelerators(AppInstance,MAKEINTRESOURCE(idr_accel));
	}

	BOOL PreTranslateMessage (MSG *msg)
	{
		BOOL used = FALSE;
		if (hAccTable)
		{
			used = ::TranslateAccelerator(hWnd,hAccTable,msg);
		}
		return used;
	}

	void destroy (void)
	{
		if (hWnd)
		{
			DestroyWindow(hWnd);
			hWnd = 0;
		}
	}

	~CWnd (void)
	{
		destroy();

		UnRegister(this);
	}

	bool Create (LPCTSTR ClassName, LPCTSTR WindowName, bool menu, DWORD dwStyle,
		const RECT &rect, HWND parent, UINT nID);

	bool create (HWND parent, const char *title, POINT *pos, int w, int h, const char *class_name="AppClass");

	bool is_valid (void)
	{
		return hWnd != 0;
	}

	virtual void OnActivate (bool b)
	{
		active = b;
	}

	virtual void OnCreate (CREATESTRUCT *cs)
	{
	}

	virtual void OnDestroy (void)
	{
	}

	virtual void OnErase (HDC dc)
	{
	}

	virtual void OnPaint (HDC dc)
	{
	}
	void Redraw (void)
	{
		HDC dc = GetDC(hWnd);
		OnPaint(dc);
		ReleaseDC(hWnd,dc);
	}


	virtual void OnClose (void)
	{
		destroy();
	}

	virtual void OnSize (int size_type, int w, int h)			{}

	virtual void OnCommand (int wNotifyCode, int wID, HWND wnd)	{}
	virtual void OnNotify (int idCtrl, LPNMHDR phdr)			{}
	virtual void OnParentNotify (int fwEvent, int lParam)		{}

	virtual void OnKeyDown (int key, int shifts)				{}
	virtual void OnKeyUp (int key, int shifts)					{}

	virtual void OnSysKeyDown (int key)							{}
	virtual void OnSysKeyUp (int key)							{}

	virtual void OnMouseMove (int x, int y, int shifts)			{}
	virtual void OnNcMouseMove (int x, int y, int hit_test)		{}

	virtual void OnLButtonDown (int x, int y, int shifts)		{}
	virtual void OnLButtonUp (int x, int y, int shifts)			{}
	virtual void OnLButtonDblClk (int x, int y, int shifts)		{}

	virtual void OnRButtonDown (int x, int y, int shifts)		{}
	virtual void OnRButtonUp (int x, int y, int shifts)			{}
	virtual void OnRButtonDblClk (int x, int y, int shifts)		{}

	virtual int OnMessage (UINT message, WPARAM wParam, LPARAM lParam)	{ return 0; }

	void SetStyle (int s)
	{
	}

	void SetText (const char *txt)
	{
		::SetWindowText(hWnd,txt);
	}

	void MoveTo (int x, int y)
	{
		SetWindowPos(hWnd,0,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}

	void Invalidate (bool erase=false)
	{
		::InvalidateRect(hWnd,0,erase);
	}
	void InvalidateRect (RECT *rect, bool erase=false)
	{
		::InvalidateRect(hWnd,rect,erase);
	}

	void GetWindowRect (RECT *r)
	{
		::GetWindowRect(hWnd,r);
	}
	virtual void GetClientRect (RECT *r)	// allow VIRTUAL override for tool/status bars etc.
	{
		::GetClientRect(hWnd,r);
	}

	int GetWidth (void)
	{
		RECT r;
		GetWindowRect(&r);
		return r.right - r.left + 1;
	}
	int GetHeight (void)
	{
		RECT r;
		GetWindowRect(&r);
		return r.bottom - r.top + 1;
	}

	int GetClientWidth (void)
	{
		RECT r;
		GetClientRect(&r); // not inclusive
		return r.right - r.left;// + 1;
	}
	int GetClientHeight (void)
	{
		RECT r;
		GetClientRect(&r); // not inclusive
		return r.bottom - r.top;// + 1;
	}

	void Activate (void)
	{
		::SetFocus(hWnd);
	}

	int SendMessage (int msg, WPARAM w=0, LPARAM l=0)
	{
		return ::SendMessage(hWnd,msg,w,l);
	}
};

//---------------------------------------------------------------------------

#endif // WINDOW_H
