//////////////////////////////////////////////////////////////////////////////
//  File:    WatchBar.cpp
//
//  Purpose: This is ia docking window that provides a watch window.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

// WatchBar.cpp : implementation file
//

#include "stdafx.h"
#include "DbgRemote.h"
#include "WatchBar.h"
#include "DataDisplayDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWatchBar

CWatchBar::CWatchBar()
{
	m_nListCol1Size = 0;
}

CWatchBar::~CWatchBar()
{
}


BEGIN_MESSAGE_MAP(CWatchBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CWatchBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CWatchBar message handlers
int CWatchBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    DWORD dwStyle = WS_CHILD|WS_BORDER|WS_VISIBLE|LVS_SINGLESEL|LVS_REPORT|
		            LVS_EDITLABELS|/*LVS_NOLABELWRAP|*/LVS_NOSORTHEADER;

	if (! m_WatchList.Create(dwStyle, CRect(0,0,10,10), this, 1010) )
		return -1;

	m_WatchList.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_LABELTIP);

	m_WatchList.InsertColumn(0, "Name", LVCFMT_LEFT, -1, -1);
	m_WatchList.InsertColumn(1, "Value", LVCFMT_LEFT, -1, -1);
	m_WatchList.InsertItem(0, "");

	m_WatchList.SetColumnWidth( 0, LVSCW_AUTOSIZE_USEHEADER);
	m_WatchList.SetColumnWidth( 1, LVSCW_AUTOSIZE_USEHEADER);

	m_WatchList.EnableEdit(0);
	m_WatchList.EnableEdit(1);
	
	if (!m_Font.CreatePointFont(100, "Courier"))
        return -1;

    m_WatchList.SetFont(&m_Font);


	//m_hBkBrush = ::CreateSolidBrush(RGB(0,0,128));
    return 0;
}

void CWatchBar::OnSize(UINT nType, int cx, int cy) 
{
    CSizingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);

    m_WatchList.MoveWindow(rc);

	if (m_nListCol1Size != 0)
	{
		m_WatchList.SetColumnWidth( 0, m_nListCol1Size);
	}
}

HBRUSH CWatchBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	/*if (nCtlColor == CTLCOLOR_EDIT)
	{
		pDC->SetBkColor(RGB(0, 0, 128)); // change the background color
		pDC->SetTextColor(RGB(255,255,255)); // change the text color

		return m_hBkBrush;
	}*/
	
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CWatchBar::OnClose() 
{
	//DeleteObject(m_hBkBrush);
	CSizingControlBar::OnClose();
}

void CWatchBar::OnWatchListDelete()
{
	POSITION pos = m_WatchList.GetFirstSelectedItemPosition();
	if (pos)
	{
		int nSel = m_WatchList.GetNextSelectedItem(pos);
		
		//don't delete the empty last item
		if (nSel < m_WatchList.GetItemCount() - 1)
			m_WatchList.DeleteItem(nSel);
	}
}

void CWatchBar::OnWatchEndLabelEdit(int nItem, int nSubItem, const char* cszNewText)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	
	if (nSubItem == 0)
	{
		//user has changed the "name" field - refresh that watch item
		int BeginLen = m_WatchList.GetItemText(nItem, 0).GetLength();
		int NewLen = 0;
		if (cszNewText)
			NewLen = strlen(cszNewText);

		if (BeginLen == 0)
		{
			//there was nothing there to begin with...
			if (NewLen != 0)
			{
				m_WatchList.SetItemText(nItem, 0, cszNewText);
				m_WatchList.InsertItem(m_WatchList.GetItemCount(), "");

				pApp->UpdateWatch(nItem);
			}
		}
		else
		{
			if (NewLen == 0)
				m_WatchList.DeleteItem(nItem);
			else
			{
				m_WatchList.SetItemText(nItem, 0, cszNewText);
				pApp->UpdateWatch(nItem);
			}
		}
	}
	else
	{
		//user has modified a value
		CString Item = m_WatchList.GetItemText(nItem, 0);
		CString Value = cszNewText;
		if (strlen(Value) == 0)
			Value = "None";
		
		pApp->ModifyVariable(Item, Value);

	}	
} // OnWatchEndLabelEdit

void CWatchBar::OnListDoubleClick()
{
	POSITION pos = m_WatchList.GetFirstSelectedItemPosition();
	if (pos)
	{
		CDataDisplayDlg dlg;
		int nSel = m_WatchList.GetNextSelectedItem(pos);
		dlg.m_Expression = m_WatchList.GetItemText(nSel, 0);
		CString ValueBefore = m_WatchList.GetItemText(nSel, 1);
		dlg.m_Value = ValueBefore;

		if (dlg.DoModal() == IDOK)
		{
			if (ValueBefore != dlg.m_Value)
			{
				CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
				pApp->ModifyVariable(dlg.m_Expression, dlg.m_Value);
			}
		}
	}

}


BOOL CWatchBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	NMHDR* pNmh = (NMHDR*)lParam;
	if (pNmh->hwndFrom == m_WatchList.m_hWnd)
	{
		NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)lParam;

		switch (pNmh->code)
		{
		case LVN_KEYDOWN:
			{
				NMLVKEYDOWN* pKeyDown = (NMLVKEYDOWN*)lParam;
				if (pKeyDown->wVKey == VK_DELETE)
				{
					OnWatchListDelete();
				}
				break;
			}

		case LVN_ENDLABELEDIT:
			OnWatchEndLabelEdit(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
			break;

		case NM_DBLCLK:
			OnListDoubleClick();
			break;

		}
	}

	else if (pNmh->code == NM_RETURN)
		OnListDoubleClick();
			

	else if (pNmh->code == HDN_ENDTRACK)
	{
		NMHEADER* pNmHdr = (NMHEADER*)lParam;
		if (pNmHdr->pitem->mask == HDI_WIDTH)
		{
			CRect r;
			m_WatchList.GetWindowRect(&r);
			if (r.Width() > 1)
				m_nListCol1Size = pNmHdr->pitem->cxy;
		}
	}

	return CSizingControlBar::OnNotify(wParam, lParam, pResult);
}


int CWatchBar::GetWatchCount()
{
	return m_WatchList.GetItemCount();
}

void CWatchBar::GetWatchItem(int ItemInx, char* szItemLabel, int MaxLen)
{
	m_WatchList.GetItemText(ItemInx, 0, szItemLabel, MaxLen);
}

void CWatchBar::UpdateWatchItem(int ItemInx, const char* cszItemData)
{
	m_WatchList.SetItemText(ItemInx, 1, cszItemData);
}

void CWatchBar::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{	
	CSizingControlBar::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CWatchBar::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	
	CSizingControlBar::OnChar(nChar, nRepCnt, nFlags);
}


void CWatchBar::AddWatchItem(const char* cszWatch)
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	int nNewItem = m_WatchList.InsertItem(m_WatchList.GetItemCount()-1, cszWatch);
	pApp->UpdateWatch(nNewItem);
}


void CWatchBar::LoadState(LPCTSTR lpszProfileName)
{
	CSizingControlBarG::LoadState(lpszProfileName);

	CWinApp* pApp = AfxGetApp();
    TCHAR szSection[256];
    wsprintf(szSection, _T("%s-SCBar-%d"), lpszProfileName, GetDlgCtrlID());

    m_nListCol1Size = pApp->GetProfileInt(szSection, _T("ListCol1Wd"), m_nListCol1Size);
}

void CWatchBar::SaveState(LPCTSTR lpszProfileName)
{
	CSizingControlBarG::SaveState(lpszProfileName);

	CWinApp* pApp = AfxGetApp();
    TCHAR szSection[256];
    wsprintf(szSection, _T("%s-SCBar-%d"), lpszProfileName, GetDlgCtrlID());

    m_nListCol1Size = pApp->WriteProfileInt(szSection, _T("ListCol1Wd"), m_nListCol1Size);
}
