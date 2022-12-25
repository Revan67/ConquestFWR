//////////////////////////////////////////////////////////////////////////////
//  File:    DataDisplayDlg.cpp
//
//  Purpose: This implements a QuickView type dialog box for viewing and modifying
//  watch items.
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
#include "dbgremote.h"
#include "DataDisplayDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDataDisplayDlg dialog


CDataDisplayDlg::CDataDisplayDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDataDisplayDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDataDisplayDlg)
	m_Expression = _T("");
	m_Value = _T("");
	//}}AFX_DATA_INIT
}


void CDataDisplayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDataDisplayDlg)
	DDX_Text(pDX, IDC_EXPR_EDT, m_Expression);
	DDX_Text(pDX, IDC_VALUE_EDT, m_Value);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDataDisplayDlg, CDialog)
	//{{AFX_MSG_MAP(CDataDisplayDlg)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDataDisplayDlg message handlers

void CDataDisplayDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	CWnd* pWnd = GetDlgItem(IDC_VALUE_EDT);
	if (pWnd)
	{
		CRect r;
		pWnd->GetWindowRect(&r);
		ScreenToClient(&r);
		r.right = cx - r.left;
		r.bottom = cy - r.left;
		pWnd->MoveWindow(&r); 
	}
}
