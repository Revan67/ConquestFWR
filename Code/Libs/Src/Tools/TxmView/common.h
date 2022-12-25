#ifndef COMMON_H
#define COMMON_H

#include <commctrl.h>	// InitCommonControls() Requires: comctl32.lib 
#include "window.h"

//---------------------------------------------------------------------------
// Tracker
//---------------------------------------------------------------------------

/*
	NOTE:	if you don't like the drag tolerance try...

		SystemParametersInfo(SPI_SETDRAGWIDTH,2,0,SPIF_SENDCHANGE);
		SystemParametersInfo(SPI_SETDRAGHEIGHT,2,0,SPIF_SENDCHANGE);

		int cx = GetSystemMetrics(SM_CXDRAG);
		int cy = GetSystemMetrics(SM_CYDRAG);
*/

struct Tracker
{
	HWND hWnd;		// owner window
	HDC hdc;		// Device Context of DESKTOP window

	bool visible;
	bool dragging;
	bool xflip;
	bool yflip;
	bool place;

	int xmin;
	int ymin;

	POINT start;		// desktop coordinates
	POINT last;

	RECT frame;			// desktop coordinates
	RECT client_frame;	// client coordinates

	POINT client_offset;

	POINT place_offset;
	POINT place_size;

	Tracker (void)
	{
		hWnd = 0;
		dragging = false;
		visible = false;
		xflip = true;
		yflip = true;
		place = false;
		xmin = 4;
		ymin = 4;
	}

	void EndResize()
	{
	}

	void Stretch (POINT p)
	{
	}

	void recalc_frame (void)
	{
		if (place)
		{
			frame.left = last.x - place_offset.x;
			frame.top = last.y - place_offset.y;
			frame.right = frame.left + place_size.x;
			frame.bottom = frame.top + place_size.y;
			return;
		}

		frame.left = start.x;
		frame.top = start.y;
		frame.right = last.x;
		frame.bottom = last.y;

		if (frame.left > frame.right)
		{
			if (xflip)
			{
				int l = frame.right;
				frame.right = frame.left+1;
				frame.left = l;
			}
			else
			{
				frame.right = frame.left + xmin;
			}
		}

		if (frame.top > frame.bottom)
		{
			if (yflip)
			{
				int t = frame.bottom;
				frame.bottom = frame.top+1;
				frame.top = t;
			}
			else
			{
				frame.bottom = frame.top + ymin;
			}
		}

		if ((frame.right-frame.left) < xmin)
		{
			frame.right = frame.left + xmin;
		}
		if ((frame.bottom - frame.top) < ymin)
		{
			frame.bottom = frame.top + ymin;
		}
	}

	void DrawFocusRect (bool erase=false)
	{
		if (visible)
		{
			::DrawFocusRect(hdc,&frame);
			visible = false;
		}

		if (!erase)
		{
			recalc_frame();
			::DrawFocusRect(hdc,&frame);
			visible = true;
		}
	}

	void Move (POINT p)
	{
		last = p;
		DrawFocusRect();
	}

	void InitLoop (void)
	{
		// handle pending WM_PAINT messages
		MSG msg;
		while (::PeekMessage(&msg, NULL, WM_PAINT, WM_PAINT, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, WM_PAINT, WM_PAINT))
				return;
			DispatchMessage(&msg);
		}

		HWND pWnd = GetDesktopWindow();
#if 0
		if (LockWindowUpdate(pWnd))
			hdc = GetDCEx(pWnd, NULL, DCX_WINDOW|DCX_CACHE|DCX_LOCKWINDOWUPDATE);
		else
#endif
			hdc = GetDCEx(pWnd, NULL, DCX_WINDOW|DCX_CACHE);


	// WINDOW COORDINATES -> CLIENT COORDINATES

		client_offset.x = 0;
		client_offset.y = 0;
		ClientToScreen(hWnd,&client_offset);
	}

	void CancelLoop (void)
	{
		DrawFocusRect(true);	// gets rid of focus rect
		ReleaseCapture();

		HWND pWnd = GetDesktopWindow();
		LockWindowUpdate(0);			// UNLOCK
		if (hdc)
		{
			ReleaseDC(pWnd,hdc);
			hdc = 0;
		}

		client_frame.left = frame.left - client_offset.x;
		client_frame.right = frame.right - client_offset.x;
		client_frame.top = frame.top - client_offset.y;
		client_frame.bottom = frame.bottom - client_offset.y;

		place = false;
	}

	bool StartDrag (POINT p)
	{
		InitLoop();

		p.x += client_offset.x;
		p.y += client_offset.y;

		start = p;

		dragging = true;

		Move(start);

		return Track() != 0;
	}
	void EndDrag()
	{
		CancelLoop();
	}

	void OnKey (int key, BOOL b)
	{
	}

	bool contains (int x, int y)
	{
		return (x >= client_frame.left && x <= client_frame.right &&
				y >= client_frame.top && y <= client_frame.bottom);
	}

	BOOL Track (void)
	{
		// don't handle if capture already set
		if (::GetCapture() != NULL)
			return FALSE;

		// set capture to the window which received this message
		SetCapture(hWnd);
		assert(GetCapture() == hWnd);

		// get messages until capture lost or cancelled/accepted
		while (GetCapture() == hWnd)
		{
			MSG msg;
			if (!::GetMessage(&msg, NULL, 0, 0))
			{
				PostQuitMessage(msg.wParam);
				break;
			}

			switch (msg.message)
			{
				case WM_LBUTTONDOWN:
					// ignore... key-initiated drags
					break;

				case WM_LBUTTONUP:
					if (dragging)
						EndDrag();
					else
						EndResize();
					return TRUE;

				case WM_RBUTTONDOWN:
					CancelLoop();
					return FALSE;

				case WM_MOUSEMOVE:
					if (dragging)
						Move(msg.pt);
					else
						Stretch(msg.pt);
					break;

				case WM_KEYUP:
					if (dragging)
						OnKey((int)msg.wParam, FALSE);
					switch (msg.wParam)
					{
						case VK_ESCAPE:	CancelLoop(); return FALSE;
						case VK_RETURN:	CancelLoop(); return TRUE;
					}
					break;
				case WM_KEYDOWN:
				{
					if (dragging)
						OnKey((int)msg.wParam, TRUE);
					switch (msg.wParam)
					{
						case VK_UP:		last.y -= 1; Move(last); break;
						case VK_DOWN:	last.y += 1; Move(last); break;
						case VK_LEFT:	last.x -= 1; Move(last); break;
						case VK_RIGHT:	last.x += 1; Move(last); break;
					}
					break;
				}

				// just dispatch rest of the messages
				default:
					DispatchMessage(&msg);
					break;
			} // switch

		} // while

        CancelLoop();

        return FALSE;
	}

	bool track (HWND h, int x, int y, bool force=false)
	{
		hWnd = h;

		POINT p;
		p.x = x;
		p.y = y;

		POINT sp = p;
		ClientToScreen(hWnd,&sp);

		if (force || DragDetect(hWnd,sp))
		{
			return StartDrag(p);
		}

		return false;
	}

	bool track_place (HWND h, int x, int y, RECT &box, bool force=false)
	{
		hWnd = h;

		POINT p;
		p.x = x;
		p.y = y;

		POINT sp = p;
		ClientToScreen(hWnd,&sp);

		if (force || DragDetect(hWnd,sp))
		{
			place = true;
			place_offset.x = x - box.left;
			place_offset.y = y - box.top;
			place_size.x = box.right - box.left + 1;
			place_size.y = box.bottom - box.top + 1;
			return StartDrag(p);
		}

		return false;
	}
};

//---------------------------------------------------------------------------
// CMenu
//---------------------------------------------------------------------------

struct CMenu
{
	HMENU hMenu;
	HMENU sub;

	CMenu (void)
	{
		hMenu = 0;
		sub = 0;
	}

	HMENU LoadMenu (int idr)
	{
		hMenu = ::LoadMenu(AppInstance,MAKEINTRESOURCE(idr));
		return hMenu;
	}

	CMenu *GetSubMenu (int pos)
	{
		sub = ::GetSubMenu(hMenu,pos);
		return this; // KLUDGE!
	}

	BOOL TrackPopupMenu (int flags, int x, int y, HWND parent)
	{
		//WARNING: call GetSubMenu() before
		return ::TrackPopupMenu(sub,flags,x,y,0,parent,0);
	}
};

//---------------------------------------------------------------------------
// CTreeCtrl
//---------------------------------------------------------------------------

struct CTreeCtrl : CWnd
{
	int id;

	bool Create (DWORD dwStyle, const RECT &rect, HWND parent, UINT nID)
	{
		dwStyle |=  WS_VISIBLE | WS_CHILD | WS_BORDER;
		dwStyle |= LBS_NOTIFY;
		dwStyle |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;
		return CWnd::Create(WC_TREEVIEW,"Tree View",false,dwStyle,rect,parent,nID);
	}

	CTreeCtrl (void)
	{
		id = 0;
	}

	HTREEITEM GetNextItem (HTREEITEM hitem, int code)
	{
		return TreeView_GetNextItem(hWnd, hitem, code);
	}
	HTREEITEM GetSelectedItem (void)
	{
		return GetNextItem(0,TVGN_CARET);
	}

	HTREEITEM GetChild (HTREEITEM hitem)		{ return GetNextItem(hitem, TVGN_CHILD); }
	//HTREEITEM GetNextSibling (HTREEITEM hitem)	{ return GetNextItem(hitem, TVGN_NEXT); }
	//HTREEITEM GetPrevSibling (HTREEITEM hitem)	{ return GetNextItem(hitem, TVGN_PREVIOUS); }
	HTREEITEM GetParent (HTREEITEM hitem)		{ return GetNextItem(hitem, TVGN_PARENT); }
	HTREEITEM GetFirstVisible (void)			{ return GetNextItem(NULL,  TVGN_FIRSTVISIBLE); }
	HTREEITEM GetNextVisible (HTREEITEM hitem)	{ return GetNextItem(hitem, TVGN_NEXTVISIBLE); }
	HTREEITEM GetPrevVisible (HTREEITEM hitem)	{ return GetNextItem(hitem, TVGN_PREVIOUSVISIBLE); }
	HTREEITEM GetSelection (void)				{ return GetNextItem(NULL,  TVGN_CARET); }
	HTREEITEM GetDropHilight (void)				{ return GetNextItem(NULL,  TVGN_DROPHILITE); }
	HTREEITEM GetRoot (void)					{ return GetNextItem(NULL,  TVGN_ROOT); }


	UINT GetCount (void)
	{
		return TreeView_GetCount(hWnd);
	}

	BOOL DeleteAllItems (void)
	{
		return TreeView_DeleteAllItems(hWnd);
	}

	HTREEITEM InsertItem (LPCTSTR lpszItem, HTREEITEM hParent=TVI_ROOT, HTREEITEM hInsertAfter=TVI_LAST)
	{
		TV_INSERTSTRUCT insert;
		insert.hParent = hParent;
		insert.hInsertAfter = hInsertAfter;
		insert.item.mask = TVIF_TEXT;
		insert.item.pszText = (char *)lpszItem;
		insert.item.cchTextMax = 0;
		insert.item.lParam = 0;
		return (HTREEITEM)SendMessage(TVM_INSERTITEM,0,(DWORD)&insert);
	}

	BOOL SetItem (HTREEITEM hItem, UINT nMask, LPCTSTR lpszItem, int nImage, int nSelectedImage, UINT nState, UINT nStateMask, LPARAM lParam)
	{
		TV_ITEM item;
		item.hItem = hItem;
		item.mask = nMask;
		item.pszText = (LPTSTR) lpszItem;
		item.iImage = nImage;
		item.iSelectedImage = nSelectedImage;
		item.state = nState;
		item.stateMask = nStateMask;
		item.lParam = lParam;
		return (BOOL)SendMessage(TVM_SETITEM, 0, (LPARAM)&item);
	}
	BOOL GetItem (TV_ITEM *item)
	{
		return (BOOL)SendMessage(TVM_GETITEM, 0, (LPARAM)item);
	}

	BOOL SetItemData (HTREEITEM i, DWORD data)
	{
		return SetItem(i,TVIF_PARAM,NULL,0,0,0,0,data);
	}
	DWORD GetItemData (HTREEITEM i)
	{
		TV_ITEM item;
		item.hItem = i;
		item.mask = TVIF_PARAM;
		if (GetItem(&item))
		{
			return item.lParam;
		}
		return 0;
	}

	BOOL Expand (HTREEITEM hItem, UINT nCode)
	{
		return TreeView_Expand(hWnd,hItem,nCode);
	}
};

//---------------------------------------------------------------------------
// CListBox
//---------------------------------------------------------------------------

struct CListBox : CWnd
{
	int id;

	HWND parent;

	CListBox (void)
	{
		hWnd = 0;
		id = 0;
		parent = 0;
	}

	bool setup (HWND _parent, int idc_list)
	{
		parent = _parent;
		id = idc_list;

		hWnd = GetDlgItem(parent,id);
		return (hWnd != 0);
	}

	int SendDlgItemMessage (int msg, WPARAM w, LPARAM l)
	{
		return ::SendDlgItemMessage(parent,id,msg,w,l);
	}

	int AddString (const char *string)
	{
		return SendDlgItemMessage(LB_ADDSTRING,0,(int)string);
	}

	void ResetContent (void)
	{
		SendDlgItemMessage(LB_RESETCONTENT,0,0);
	}

	int GetCurSel (void)
	{
		return SendDlgItemMessage(LB_GETCURSEL,0,0);
	}

	int SetItemData (int i, DWORD data)
	{
		return SendDlgItemMessage(LB_SETITEMDATA,i,data);
	}

	int GetItemData (int i)
	{
		return SendDlgItemMessage(LB_GETITEMDATA,i,0);
	}
};

//---------------------------------------------------------------------------
// CStatusBar
//---------------------------------------------------------------------------

struct CStatusBar : CWnd
{
	bool Create (DWORD style, HWND parent, UINT nID)
	{
		style |=  WS_VISIBLE | WS_CHILD;
		hWnd = CreateStatusWindow(style,"Status Bar",parent,nID);
		return (hWnd != 0);
	}

	CStatusBar (void)
	{
	}
};

//---------------------------------------------------------------------------
// CDocument
//---------------------------------------------------------------------------

#define NONAME "noname"		// default un-named document

class CDocument : public CWnd
{
private:

	bool			modified;

public:

	TString<128>	app_name;
	TString<128>	doc_file;

	void set_app_name (const char *name)
	{
		app_name = name;
	}
	void set_doc_file (const char *name)
	{
		doc_file = name;
		update_name();
	}

	void update_name (void)
	{
		char m = modified ? '*' : ' ';
		char work[128];
		sprintf(work,"%s - [%s%c]",(const char *)app_name,(const char *)doc_file,m);
		SetText(work);
	}
	void update_document (void)
	{
		//modified = false;
		update_name();
	}

	CDocument (void)
	{
		modified = false;
		app_name = "App";
		doc_file = NONAME;
	}

	bool IsModified (void) const
	{
		return modified;
	}

	void SetModifiedFlag (bool b=true)
	{
		if (b != modified)
		{
			modified = b;
			update_name();
		}
	}

	virtual bool SaveModifications (void)
	{
		// FUTURE: put up save dialog etc.
		// modified = false;
		return false;
	}

	bool OnSave (void)
	{
		if (modified)
		{
			char msg[128];
			sprintf(msg,"Save changes to %s?",(const char *)doc_file);

			int r = MessageBox(hWnd,msg,app_name,MB_YESNOCANCEL|MB_ICONEXCLAMATION);
			switch (r)
			{
			case IDYES:
				return SaveModifications();

			case IDNO:
				return true;

			default:
				return false;
			}
		}

		return true;
	}

	void OnClose (void)
	{
		if (OnSave())
		{
			destroy();
		}
	}
};

//---------------------------------------------------------------------------
// ToopTip
//---------------------------------------------------------------------------

struct ToolTip : CWnd
{
	bool Create (HWND parent=0, int style=0)
	{
		hWnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL,
				WS_POPUP | style, // force WS_POPUP
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				parent,NULL,AppInstance,this);

		return hWnd != 0;
	}

	void RelayEvent (MSG *pMsg)
	{
        // transate the message based on TTM_WINDOWFROMPOINT
        MSG msg = *pMsg;
        //msg.hwnd = (HWND)SendMessage(TTM_WINDOWFROMPOINT, 0, (LPARAM)&msg.pt);
		msg.hwnd = pMsg->hwnd;
/*
        POINT pt = pMsg->pt;
        if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST)
                ::ScreenToClient(msg.hwnd, &pt);
        msg.lParam = MAKELONG(pt.x, pt.y);
*/
        // relay mouse event before deleting old tool
        SendMessage(TTM_RELAYEVENT, 0, (LPARAM)&msg);
	}

/*
	TOOL

	case TTN_NEEDTEXT:

	// on MouseMove()
	// ToopTip.Activate(TRUE);
*/

	void Activate (bool bEnable)
	{
		SendMessage(TTM_ACTIVATE, bEnable);
	}

	void FillInToolInfo (TOOLINFO& ti, HWND wnd, UINT nIDTool) const
	{
			memset(&ti, 0, sizeof(TOOLINFO));
			ti.cbSize = sizeof(TOOLINFO);
			if (nIDTool == 0)
			{
					ti.hwnd = ::GetParent(wnd);
					ti.uFlags = TTF_IDISHWND;
					ti.uId = (UINT)wnd;
			}
			else
			{
					ti.hwnd = wnd;
					ti.uFlags = 0;
					ti.uId = nIDTool;
			}
	}

	void UpdateTipText (LPCTSTR lpszText, HWND parent, UINT nIDTool=0)
	{
		assert(::IsWindow(hWnd));
		assert(parent != 0);

		TOOLINFO ti;
		FillInToolInfo(ti, parent, nIDTool);
		ti.lpszText = (LPTSTR)lpszText;
		SendMessage(TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
	}

	void GetMargin (RECT *rect)
	{
		SendMessage(TTM_GETMARGIN, 0, (LPARAM)rect);
	}

	void SetMargin (RECT *rect)
	{
		SendMessage(TTM_SETMARGIN, 0, (LPARAM)rect);
	}

	bool AddTool (HWND parent, const char *lpszText, RECT *lpRectTool, UINT nIDTool)
	{
		assert(::IsWindow(hWnd));
		assert(parent != NULL);
		assert(lpszText != NULL);
		// the toolrect and toolid must both be zero or both valid
		assert((lpRectTool != NULL && nIDTool != 0) ||
				   (lpRectTool == NULL) && (nIDTool == 0));

		TOOLINFO ti;
		FillInToolInfo(ti, parent, nIDTool);
		if (lpRectTool != NULL)
				memcpy(&ti.rect, lpRectTool, sizeof(RECT));
		ti.lpszText = (LPTSTR)lpszText;
		return SendMessage(TTM_ADDTOOL, 0, (LPARAM)&ti) != 0;
	}
};

/*
inline void CenterWindow (HWND hWnd, int width, int height)
{
	RECT rect;

	GetWindowRect(GetDesktopWindow(),&rect);
	int DESK_W = rect.right;
	int DESK_H = rect.bottom;

	rect.left = (DESK_W - width) / 2;
	rect.top = (DESK_H - height) / 2;
	rect.right = rect.left + width - 1;
	rect.bottom = rect.top + height - 1;

	AdjustWindowRectEx (&rect,
		GetWindowLong(hWnd, GWL_STYLE),
		(GetMenu(hWnd) != NULL),
		GetWindowLong(hWnd, GWL_EXSTYLE));

	SetWindowPos (hWnd, 
		HWND_TOP, 
		rect.left,rect.top,
		rect.right  - rect.left + 1,
		rect.bottom - rect.top  + 1,
		SWP_NOCOPYBITS | SWP_NOZORDER);
}
*/

//---------------------------------------------------------------------------

#endif //COMMON_H
