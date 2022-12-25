//////////////////////////////////////////////////////////////////////////////
//  File:    VariableViewBar.cpp
//
//  Purpose: This is a docking window that provides a display of global and
//  local python variables.
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

#include "stdafx.h"
#include "DbgRemote.h"
#include "VariableViewBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVariableViewBar

CVariableViewBar::CVariableViewBar()
{
	m_nListColWidth = 0;
}

CVariableViewBar::~CVariableViewBar()
{
}


BEGIN_MESSAGE_MAP(CVariableViewBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CVariableViewBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CVariableViewBar message handlers
int CVariableViewBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    DWORD dwStyle = WS_CHILD|WS_BORDER|LVS_SINGLESEL|LVS_REPORT|
		            LVS_EDITLABELS|LVS_NOLABELWRAP|LVS_NOSORTHEADER|LVS_SINGLESEL|
					LVS_SORTASCENDING;
	
	if (! m_LocalsList.Create(dwStyle, CRect(0,0,10,10), this, 1010) )	
		return -1;
	

	dwStyle |= WS_VISIBLE;
	if (! m_GlobalsList.Create(dwStyle, CRect(0,0,10,10), this, 1010) )	
		return -1;

	
	m_LocalsList.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_LABELTIP);
	m_LocalsList.EnableEdit(1);
	m_GlobalsList.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_LABELTIP);
	m_GlobalsList.EnableEdit(1);

	m_LocalsList.InsertColumn(0, "Name", LVCFMT_LEFT, -1, -1);
	m_LocalsList.InsertColumn(1, "Value", LVCFMT_LEFT, -1, -1);
	m_LocalsList.SetColumnWidth( 0, LVSCW_AUTOSIZE_USEHEADER);
	m_LocalsList.SetColumnWidth( 1, LVSCW_AUTOSIZE_USEHEADER);

	m_GlobalsList.InsertColumn(0, "Name", LVCFMT_LEFT, -1, -1);
	m_GlobalsList.InsertColumn(1, "Value", LVCFMT_LEFT, -1, -1);
	
	m_GlobalsList.SetColumnWidth( 0, LVSCW_AUTOSIZE_USEHEADER);
	m_GlobalsList.SetColumnWidth( 1, LVSCW_AUTOSIZE_USEHEADER);

	m_Tab.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|TCS_TABS|TCS_BOTTOM|TCS_RIGHTJUSTIFY, CRect(0,0,10,10), this, 1011);
	m_Tab.SetMinTabWidth(0);
	m_Tab.InsertItem(TCIF_TEXT, 0, "Globals", 0, 0);
	m_Tab.InsertItem(TCIF_TEXT, 1, "Locals", 0, 0);
	m_Tab.SetCurSel(0);

	if (!m_Font.CreatePointFont(100, "Courier"))
        return -1;

    m_LocalsList.SetFont(&m_Font);
	m_GlobalsList.SetFont(&m_Font);
	m_Tab.SetFont(&m_Font);

	m_hBkBrush = ::CreateSolidBrush(RGB(0,0,128));
    return 0;
}




void CVariableViewBar::OnSize(UINT nType, int cx, int cy) 
{
    CSizingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);
	
	m_Tab.MoveWindow(rc);
	m_Tab.AdjustRect(FALSE, &rc);
	rc.DeflateRect(2, 2);

	m_LocalsList.MoveWindow(rc);
	m_GlobalsList.MoveWindow(rc);
	
	if (m_nListColWidth != 0)
	{
		m_LocalsList.SetColumnWidth( 0, m_nListColWidth);
		m_GlobalsList.SetColumnWidth( 0, m_nListColWidth);
	}
}





HBRUSH CVariableViewBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{

	
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CVariableViewBar::OnClose() 
{
	DeleteObject(m_hBkBrush);
	CSizingControlBar::OnClose();
}


void CVariableViewBar::UpdateGlobals(const char* szGlobals)
{
	//m_GlobalsList.LockWindowUpdate();
	m_GlobalsList.DeleteAllItems();

	int itemInx = 0;
	assert(szGlobals);
	const char* pCur = szGlobals;
	CString CurItem, Name, Value;
	while (strlen(pCur) > 0)
	{
		CurItem = pCur;
		pCur += strlen(pCur)+1;

		int i = CurItem.Find(",");
		if (i > 0)
		{
			Name = CurItem.Left(i);
			Value = CurItem.Right(CurItem.GetLength() - i - 1);
		}

		int nNewItemInx = m_GlobalsList.InsertItem(itemInx++, Name);
		if (Value.GetLength() > 0)
		{
			m_GlobalsList.SetItemText(nNewItemInx, 1, Value);
		}
	}
	
	//m_GlobalsList.UnlockWindowUpdate();
}

void CVariableViewBar::UpdateLocals(const char* szLocals)
{
	//m_LocalsList.LockWindowUpdate();
	m_LocalsList.DeleteAllItems();
	
	assert(szLocals);
	
	const char* pCur = szLocals;
	
	CString CurItem, Name, Value;
	while (strlen(pCur) > 0)
	{
		CurItem = pCur;
		pCur += strlen(pCur)+1;

		int i = CurItem.Find(",");
		if (i > 0)
		{
			Name = CurItem.Left(i);
			Value = CurItem.Right(CurItem.GetLength() - i - 1);
		}

		int nNewItemInx = m_LocalsList.InsertItem(0, Name);
		if (Value.GetLength() > 0)
		{
			m_LocalsList.SetItemText(nNewItemInx, 1, Value);
		}
	}
	//m_LocalsList.UnlockWindowUpdate();
}




void CVariableViewBar::ShowGlobals()
{
//	LockWindowUpdate();
	m_GlobalsList.ShowWindow(SW_SHOW);
	m_LocalsList.ShowWindow(SW_HIDE);
//	UnlockWindowUpdate();
}

void CVariableViewBar::ShowLocals()
{
//	LockWindowUpdate();
	m_LocalsList.ShowWindow(SW_SHOW);
	m_GlobalsList.ShowWindow(SW_HIDE);
	//	UnlockWindowUpdate();
}


BOOL CVariableViewBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* pNmHdr = (NMHDR*)lParam;

    if(pNmHdr->code == TCN_SELCHANGE)
	{
		if (m_Tab.GetCurSel() == 0)
		{
			ShowGlobals();
		}
		else
		{
			ShowLocals();
		}
	}
	else if (pNmHdr->code == LVN_ENDLABELEDIT)
	{
		NMLVDISPINFO* pdi = (NMLVDISPINFO*) lParam;
		if (m_Tab.GetCurSel() == 0)
			ModifyGlobalItem(pdi->item.iItem, pdi->item.iSubItem, pdi->item.pszText);
		else
			ModifyLocalItem(pdi->item.iItem, pdi->item.iSubItem, pdi->item.pszText);
	}
	else if (pNmHdr->code == HDN_ENDTRACK)
	{
		NMHEADER* pNmHdr = (NMHEADER*)lParam;
		if (pNmHdr->pitem->mask == HDI_WIDTH)
		{
			m_nListColWidth = pNmHdr->pitem->cxy;

			m_LocalsList.SetColumnWidth( 0, m_nListColWidth);
			m_GlobalsList.SetColumnWidth( 0, m_nListColWidth);
		}
	}
	
	
	return CSizingControlBar::OnNotify(wParam, lParam, pResult);
}



void CVariableViewBar::ModifyGlobalItem(int nItem, int nSubItem, const char* szNewVal)
{
	if (nSubItem <= 0)
		return;

	//if the user presses escape - this is null
	if (!szNewVal)
		return;

	CString PrevItemText = m_GlobalsList.GetItemText(nItem, nSubItem);
	if (strcmp(PrevItemText, szNewVal) != 0)
	{
		CString VarName = m_GlobalsList.GetItemText(nItem, 0);
		
		CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
		pApp->ModifyVariable(VarName, szNewVal);
	}	
}

void CVariableViewBar::ModifyLocalItem(int nItem, int nSubItem, const char* szNewVal)
{
	if (nSubItem <= 0)
		return;

	if (!szNewVal)
		return;

	CString PrevItemText = m_LocalsList.GetItemText(nItem, nSubItem);
	if (strcmp(PrevItemText, szNewVal) != 0)
	{
		CString VarName = m_LocalsList.GetItemText(nItem, 0);
		
		CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
		pApp->ModifyVariable(VarName, szNewVal);
	}
}
	



void CVariableViewBar::LoadState(LPCTSTR lpszProfileName)
{
	assert(lpszProfileName);

	CSizingControlBarG::LoadState(lpszProfileName);

	CWinApp* pApp = AfxGetApp();
	assert(pApp);

    TCHAR szSection[256];
    wsprintf(szSection, _T("%s-SCBar-%d"), lpszProfileName, GetDlgCtrlID());

    m_nListColWidth = pApp->GetProfileInt(szSection, _T("ListCol1Wd"), m_nListColWidth);
}

void CVariableViewBar::SaveState(LPCTSTR lpszProfileName)
{
	assert(lpszProfileName);
	CSizingControlBarG::SaveState(lpszProfileName);

	CWinApp* pApp = AfxGetApp();
    TCHAR szSection[256];
    wsprintf(szSection, _T("%s-SCBar-%d"), lpszProfileName, GetDlgCtrlID());

    pApp->WriteProfileInt(szSection, _T("ListCol1Wd"), m_nListColWidth);
}