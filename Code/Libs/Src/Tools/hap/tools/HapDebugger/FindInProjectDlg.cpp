// FindInProjectDlg.cpp : implementation file
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
#include "FindInProjectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindInProjectDlg dialog


CFindInProjectDlg::CFindInProjectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindInProjectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFindInProjectDlg)
	m_Filename = _T("");
	//}}AFX_DATA_INIT
}


void CFindInProjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindInProjectDlg)
	DDX_Text(pDX, IDC_FILENAME_EDT, m_Filename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindInProjectDlg, CDialog)
	//{{AFX_MSG_MAP(CFindInProjectDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFindInProjectDlg message handlers
