//////////////////////////////////////////////////////////////////////////////
//  File:    RunOptionsDlg.cpp
//
//  Purpose: This dialog allows you to specify options related to debugging (
//  and running) the python program.
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

// CRunOptionsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "shlwapi.h"
#include "dbgremote.h"
#include "RunOptionsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRunOptionsDlg dialog


CRunOptionsDlg::CRunOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRunOptionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRunOptionsDlg)
	m_RunParameters = _T("");
	m_RunScript = _T("");
	m_WorkingDir = _T("");
	//}}AFX_DATA_INIT
}


void CRunOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRunOptionsDlg)
	DDX_Text(pDX, IDC_RUNPARAM_EDT, m_RunParameters);
	DDX_Text(pDX, IDC_RUNSCRIPT_EDT, m_RunScript);
	DDX_Text(pDX, IDC_WORKDIR_EDT, m_WorkingDir);
	DDV_WorkingDir(pDX, m_WorkingDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRunOptionsDlg, CDialog)
	//{{AFX_MSG_MAP(CRunOptionsDlg)
	ON_BN_CLICKED(IDC_SCRIPTBROWSE_BTN, OnScriptBrowseBtn)
	ON_BN_CLICKED(IDC_WORKDIRBROWSE_BTN, OnWorkdirBrowseBtn)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRunOptionsDlg message handlers
void PASCAL CRunOptionsDlg::DDV_WorkingDir(CDataExchange* pDX, CString workingDir)
{
	// check to make sure the path is not empty. if it is, we skip the check
	// otherwise, a dialog box will pop up when the settings are opened for the first time
	if (workingDir != "")
	{
		// check to see if the path we added is valid
		if (!(PathFileExists(workingDir)))
		{
			// if not, tell them to enter a valid path and not allow the dialog to close
			::AfxMessageBox("That path is not valid", MB_OK, 0);
			pDX->Fail();
		}
	}
}

void CRunOptionsDlg::OnScriptBrowseBtn() 
{
	UpdateData(TRUE);
	CFileDialog dlg(TRUE, "*.py", m_RunScript, OFN_HIDEREADONLY, "Python Source (*.py)|*.py;*.pyc|All Files (*.*)|*.*||", this);
	if (dlg.DoModal() == IDOK)
	{
		m_RunScript = dlg.GetPathName();
	}

	UpdateData(FALSE);
}

void CRunOptionsDlg::OnWorkdirBrowseBtn() 
{
	UpdateData(TRUE);

	char WorkDir[MAX_PATH];
	strcpy(WorkDir, m_WorkingDir);

	LPMALLOC lpMalloc;  // pointer to IMalloc
	if (::SHGetMalloc(&lpMalloc) != NOERROR)
		return;


	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pszDisplayName = WorkDir;
	bi.lpszTitle = "Select Working Directory";
	bi.ulFlags = 0;
	bi.lpfn = 0;
	bi.lParam = 0;
	bi.pidlRoot = 0;

	LPITEMIDLIST lpItemIDList = SHBrowseForFolder(&bi); 

	if (lpItemIDList)
	{
		SHGetPathFromIDList(lpItemIDList, WorkDir);

		m_WorkingDir = WorkDir;
	}

	lpMalloc->Free(lpItemIDList);
	lpMalloc->Release();

	UpdateData(FALSE);
}


#define BORDER_WD	10

void CRunOptionsDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	CWnd* pCur = GetDlgItem(IDC_SCRIPTBROWSE_BTN);
	CRect r;

	int nEditRight = 0;
	if (pCur)
	{
		pCur->GetWindowRect(&r);
		ScreenToClient(&r);
		
		int nWd = r.right - r.left;
		r.right = cx - BORDER_WD;
		
		r.left = r.right - nWd;
		nEditRight = r.left - BORDER_WD;

		pCur->MoveWindow(&r, TRUE);
	}
	pCur = GetDlgItem(IDC_WORKDIRBROWSE_BTN);
	if (pCur)
	{
		pCur->GetWindowRect(&r);
		ScreenToClient(&r);
		
		int nWd = r.right - r.left;
		r.right = cx - BORDER_WD;
		
		r.left = r.right - nWd;
		pCur->MoveWindow(&r, TRUE);
	}

	pCur = GetDlgItem(IDC_RUNSCRIPT_EDT);
	if (pCur)
	{
		pCur->GetWindowRect(&r);
		ScreenToClient(&r);
		r.right = nEditRight;
		pCur->MoveWindow(&r);	
	}

	pCur = GetDlgItem(IDC_RUNPARAM_EDT);
	if (pCur)
	{
		pCur->GetWindowRect(&r);
		ScreenToClient(&r);
		r.right = nEditRight;
		pCur->MoveWindow(&r);	
	}
	
	pCur = GetDlgItem(IDC_WORKDIR_EDT);
	if (pCur)
	{
		pCur->GetWindowRect(&r);
		ScreenToClient(&r);
		r.right = nEditRight;
		pCur->MoveWindow(&r);	
	}
}
