// mybar.cpp : implementation file
//

#include "stdafx.h"
#include "SideBar.h"

#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSidebar

CSidebar::CSidebar()
{
	name = "CSidebar";
}

CSidebar::~CSidebar()
{
}


BEGIN_MESSAGE_MAP(CSidebar, CSizingControlBarG)
	//{{AFX_MSG_MAP(CSidebar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSidebar message handlers

int CSidebar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSizingControlBarG::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	SetSCBStyle(GetSCBStyle() | SCBS_SIZECHILD);

//	DWORD dwFlags = SS_BITMAP | SS_REALSIZEIMAGE | WS_CHILD | WS_VISIBLE;
//	if (!m_wndChild.Create("none",dwFlags,CRect(0,0,0,0), this))
//	{
//		return -1;
//	}
//
//	m_wndChild.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
//
//	if( m_bitmap.LoadBitmap(IDB_BITMAP1) )
//	{
//		m_wndChild.SetBitmap( m_bitmap );
//	}

	return 0;
}
