#ifndef CTREEVIEWH
#define CTREEVIEWH

#include <windows.h>
#include "commctrl.h"


class CTreeView {
	public:
		CTreeView ();
		~CTreeView ();
		SetPath (char *szPath);
		AddItem (char *szName);
		HWND GetHwnd() 
			{ return m_hWndTree; };
		BOOL Create (HWND hwndParent, int x, int y, int dx, int dy);
		void Delete();
		virtual LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_hWndTree;
		HWND m_hWndFrame;
		HTREEITEM m_hTreeRoot;
		HTREEITEM m_hCurrentItem;
		HTREEITEM m_hCurrentParent;
};

LRESULT CALLBACK CTreeViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif