// BrowseSelDlg.cpp : implementation file

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
#include "BrowseSelDlg.h"


// CBrowseSelDlg dialog

IMPLEMENT_DYNAMIC(CBrowseSelDlg, CDialog)
CBrowseSelDlg::CBrowseSelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseSelDlg::IDD, pParent)
{
}

CBrowseSelDlg::~CBrowseSelDlg()
{
}

void CBrowseSelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CBrowseSelDlg::AddBrowseItem(const char* cszFile, int nLine)
{
	m_browseItems.push_back(SBrowseItem(cszFile, nLine));
}

int CBrowseSelDlg::SelectBrowseItem()
{
	m_SelectedItem = -1;
	if (DoModal() == IDOK)
	{
		return m_SelectedItem;
	}
	return -1;
}

BEGIN_MESSAGE_MAP(CBrowseSelDlg, CDialog)
	ON_WM_SIZE()
	ON_NOTIFY(NM_DBLCLK, IDC_BROWSE_CLTL, OnNMDblclkBrowseCltl)
END_MESSAGE_MAP()


// CBrowseSelDlg message handlers

BOOL CBrowseSelDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_BROWSE_CLTL);
	assert(pLst);

	pLst->InsertColumn(0, "File");
	pLst->InsertColumn(1, "Line");
	
	char numBuff[32];
	for (unsigned int i = 0; i < m_browseItems.size(); i++)
	{
		int ItemInx = pLst->InsertItem(i, m_browseItems[i].file.c_str() );
		_snprintf(numBuff, 31, "%d", m_browseItems[i].line);
		pLst->SetItemText(ItemInx, 1, numBuff);
	}
	
	pLst->SetColumnWidth(0, LVSCW_AUTOSIZE);
	pLst->SetColumnWidth(1, LVSCW_AUTOSIZE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CBrowseSelDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_BROWSE_CLTL);
	CWnd* pBtn = GetDlgItem(IDCANCEL);
	if (pLst && pBtn)
	{
		int Border = 8;
		RECT r;
		
		pBtn->GetWindowRect(&r);
		ScreenToClient(&r);
		int wd = r.right - r.left;
		int ht = r.bottom - r.top;

		r.bottom = cy - Border;
		r.top = r.bottom - ht;
		r.left = cx / 2 - wd / 2;
		r.right = r.left + wd;

		int btnTop = r.top;

		pBtn->MoveWindow(&r);
		
		pLst->GetWindowRect(&r);
		ScreenToClient(&r);
		r.right = cx - r.left * 2;
		r.bottom = btnTop - Border;
		pLst->MoveWindow(&r);



	}
	
}

void CBrowseSelDlg::OnNMDblclkBrowseCltl(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	
	NMITEMACTIVATE* pNmA = (NMITEMACTIVATE*)pNMHDR;
	if (pNmA->iItem > -1)
	{
		m_SelectedItem = pNmA->iItem;
		OnOK();
	}
}
