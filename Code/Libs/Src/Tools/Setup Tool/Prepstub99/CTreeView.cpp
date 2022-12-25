#include "CTreeView.hpp"
#include "string.h"
#define CTREEVIEW_FRAME_CLASS	"aas:CTreeViewFrameClass"

CTreeView::Create (HWND hwndParent, int x, int y, int dx, int dy)
{
	InitCommonControls ();
	
	WNDCLASSEX wc;

	// register the CTreeViewFrame window classes

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (	(HINSTANCE)GetWindowLong (hwndParent, GWL_HINSTANCE), 
							CTREEVIEW_FRAME_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc =  (WNDPROC) CTreeViewWndFrameProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = (HINSTANCE)GetWindowLong (hwndParent, GWL_HINSTANCE);
		wc.hIcon = NULL;; 
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CTREEVIEW_FRAME_CLASS; 

		if (!RegisterClassEx (&wc))
			return false;
	}

	m_hWndFrame = CreateWindowEx (WS_EX_CLIENTEDGE, CTREEVIEW_FRAME_CLASS, "",
			WS_CHILD, 0, 0, 0, 0,
			hwndParent, (HMENU) this, (HINSTANCE)GetWindowLong (hwndParent, GWL_HINSTANCE), NULL);

	if (m_hWndFrame == NULL) 
		return false;

	m_hWndTree = CreateWindowEx (WS_EX_CLIENTEDGE,	
								WC_TREEVIEW , 
								"", 
								WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | WS_VISIBLE ,
								x, y, 
								dx, dy,
								m_hWndFrame, 
								(HMENU) this, 
								(HINSTANCE)GetWindowLong (hwndParent, GWL_HINSTANCE),
								NULL);

	if (m_hWndTree == NULL) 
		return false;

	ShowWindow (m_hWndTree, SW_SHOW);
	ShowWindow (m_hWndFrame, SW_SHOW);

	UpdateWindow (m_hWndTree);

	return true;
}

CTreeView::CTreeView ()
{
	m_hWndTree = NULL;
	m_hWndFrame = NULL;
	m_hTreeRoot = TVI_ROOT;
	m_hCurrentItem = TVI_FIRST;
	m_hCurrentParent = TVI_ROOT;
}

CTreeView::~CTreeView ()
{
	if (IsWindow(m_hWndFrame))
		DestroyWindow (m_hWndFrame);
	
	if (IsWindow(m_hWndTree)) 
		DestroyWindow (m_hWndTree);
}

CTreeView::SetPath (char *szPath)
{
	return 1;
}

CTreeView::AddItem (char *szName)
{
	TV_ITEM tvItem;
	TV_INSERTSTRUCT tvInsert;

	tvItem.mask = TVIF_TEXT | TVIF_PARAM;
	tvItem.pszText = szName;
	tvItem.cchTextMax = lstrlen (szName);
	tvItem.lParam = (LPARAM)this;

	tvInsert.hInsertAfter = m_hCurrentItem;
	tvInsert.hParent = m_hCurrentParent;
	tvInsert.item = tvItem;
	
	m_hCurrentItem = (HTREEITEM) SendMessage(m_hWndTree, TVM_INSERTITEM, 0, 
         (LPARAM) (LPTV_INSERTSTRUCT) &tvInsert);  

	m_hCurrentParent = m_hCurrentItem;

	return 1;
}


LRESULT CALLBACK CTreeViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// The pointer to the object instance is stored in the menu handle.
	// Get the instance pointer and then dispatch the message to that
	// instance.

	CTreeView *pTreeView = (CTreeView *)GetMenu (hWnd);
	
	return (pTreeView->ProcessMessages (hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK CTreeView::ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}


void CTreeView::Delete()
{
	DestroyWindow (m_hWndFrame);
	DestroyWindow (m_hWndTree);
	delete this;
}