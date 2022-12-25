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


// ToolConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "ToolConfigDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolConfigDlg dialog


CToolConfigDlg::CToolConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CToolConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CToolConfigDlg)
	m_CurToolName = _T("");
	m_CurToolCmd = _T("");
	m_CurToolDir = _T("");
	m_CurToolParams = _T("");
	//}}AFX_DATA_INIT
}


void CToolConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CToolConfigDlg)
	DDX_Text(pDX, IDC_NAME_EDT, m_CurToolName);
	DDX_Text(pDX, IDC_CMD_EDT, m_CurToolCmd);
	DDX_Text(pDX, IDC_DIR_EDT, m_CurToolDir);
	DDX_Text(pDX, IDC_PARAM_EDT, m_CurToolParams);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CToolConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CToolConfigDlg)
	ON_BN_CLICKED(IDC_BROWSECMD_BTN, OnBrowseCmdBtn)
	ON_BN_CLICKED(IDC_BROWSEDIR_BTN, OnBrowseDirBtn)
	ON_BN_CLICKED(IDC_ADD_BTN, OnAddBtn)
	ON_BN_CLICKED(IDC_REMOVE_BTN, OnRemoveBtn)
	ON_LBN_SELCHANGE(IDC_TOOLS_LST, OnSelchangeToolsLst)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CToolConfigDlg message handlers

void CToolConfigDlg::OnBrowseCmdBtn() 
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, 0, (const char*)m_CurToolCmd, OFN_HIDEREADONLY, "All Files(*.*)|*.*||", this);
	if (dlg.DoModal() == IDOK)
	{
		m_CurToolCmd = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CToolConfigDlg::OnBrowseDirBtn() 
{
	UpdateData(TRUE);
	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = 0;
	bi.lpszTitle = "Select Project Working Directory";
	bi.lpfn = 0;
	
	#ifdef _PLATFORM_SDK
	bi.ulFlags = BIF_USENEWUI|BIF_BROWSEINCLUDEFILES;
	#else
	bi.ulFlags = BIF_BROWSEINCLUDEFILES;               // Changed 10-15-02 Bruce Dickey
	#endif
	
	char Path[MAX_PATH+1];
	bi.pszDisplayName = Path;
	LPITEMIDLIST pidlFolder = SHBrowseForFolder(&bi);
	if (pidlFolder)
	{
		SHGetPathFromIDList(pidlFolder, Path);
		m_CurToolDir = Path;

		UpdateData(FALSE);
	}
}

void CToolConfigDlg::OnAddBtn() 
{
	UpdateData(TRUE);
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();

	pCfg->AddTool(m_CurToolName, m_CurToolCmd, m_CurToolDir, m_CurToolParams);

	UpdateToolsList();

	m_CurToolName = _T("");
	m_CurToolCmd = _T("");
	m_CurToolDir = _T("");
	m_CurToolParams = _T("");

	UpdateData(FALSE);
}

void CToolConfigDlg::OnRemoveBtn() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CListBox* pLst = (CListBox*)GetDlgItem(IDC_TOOLS_LST);
	assert(pLst);

	int iSel = pLst->GetCurSel();
	if (iSel > -1)
	{
		pCfg->DelTool(iSel);
		GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(FALSE);
		UpdateToolsList();
	}

}

void CToolConfigDlg::OnSelchangeToolsLst() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CListBox* pLst = (CListBox*)GetDlgItem(IDC_TOOLS_LST);
	assert(pLst);

	int iSel = pLst->GetCurSel();
	if (iSel > -1)
	{
		assert(iSel < pCfg->GetToolsCount());
		m_CurToolName = pCfg->GetToolName(iSel);
		m_CurToolCmd = pCfg->GetToolCmd(iSel);
		m_CurToolDir = pCfg->GetToolDir(iSel);
		m_CurToolParams = pCfg->GetToolArgs(iSel);

		GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(TRUE);
	}
	else
	{
		m_CurToolName = "";
		m_CurToolCmd = "";
		m_CurToolDir = "";
		m_CurToolParams = "";

		GetDlgItem(IDC_REMOVE_BTN)->EnableWindow(FALSE);
	}
	UpdateData(FALSE);
}



void CToolConfigDlg::UpdateToolsList()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CListBox* pLst = (CListBox*)GetDlgItem(IDC_TOOLS_LST);
	assert(pLst);
	
	pLst->ResetContent();
	for (int i = 0; i < pCfg->GetToolsCount(); i++)
	{
		pLst->AddString( pCfg->GetToolName(i));
	}
	pLst->SetCurSel(-1);
}

BOOL CToolConfigDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	UpdateToolsList();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
