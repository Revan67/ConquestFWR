//////////////////////////////////////////////////////////////////////////////
//  File:    BreakpointDlg.cpp
//
//  Purpose: This is a dialog which displays the current set of breakpoints
//  (currently unimplemented)
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

// BreakpointDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "BreakpointDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBreakpointDlg dialog


CBreakpointDlg::CBreakpointDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBreakpointDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBreakpointDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CBreakpointDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBreakpointDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBreakpointDlg, CDialog)
	//{{AFX_MSG_MAP(CBreakpointDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_BREAKPT_LCTRL, OnDblclkBreakptList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBreakpointDlg message handlers

BOOL CBreakpointDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_BREAKPT_LCTRL);
	pList->InsertColumn(0, "File");
	pList->InsertColumn(1, "Line");
	pList->InsertColumn(2, "Expression");
	

	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	const char_vector* pBreakPts = pApp->GetBreakpointList();

		

	pList->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	pList->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	pList->SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CBreakpointDlg::OnDblclkBreakptList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}

