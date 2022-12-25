// DocSelectDlg.cpp : implementation file

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
#include "DocSelectDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDocSelectDlg dialog


CDocSelectDlg::CDocSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDocSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDocSelectDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDocSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDocSelectDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDocSelectDlg, CDialog)
	//{{AFX_MSG_MAP(CDocSelectDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDocSelectDlg message handlers

BOOL CDocSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	
	CTreeCtrl* pTree = (CTreeCtrl*)GetDlgItem(IDC_DOCLIST_TRE);
	assert(pTree);
	for (int i = 0; i < m_aDocNames.GetSize(); i++)
		pTree->InsertItem(m_aDocNames[i]);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDocSelectDlg::OnOK() 
{
	CTreeCtrl* pTree = (CTreeCtrl*)GetDlgItem(IDC_DOCLIST_TRE);
	
	HTREEITEM hItem = pTree->GetRootItem();
	int i = 0;
	int nItemsDeleted = 0;
	while (hItem)
	{
		if (! pTree->GetCheck(hItem))
		{
			m_aDocNames.RemoveAt(i - nItemsDeleted);
			nItemsDeleted++;
			i++;
		}
		hItem = pTree->GetNextSiblingItem(hItem);
	}
	
	CDialog::OnOK();
}
