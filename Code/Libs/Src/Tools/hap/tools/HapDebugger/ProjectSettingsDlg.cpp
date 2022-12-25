//////////////////////////////////////////////////////////////////////////////
//  File:    ProjectSettingsDlg.cpp
//
//  Purpose: A dialog box that allows you to change the name (and in the future)
//  other properties of a project.
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

// ProjectSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "shlwapi.h"
#include "dbgremote.h"
#include "ProjectSettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectSettingsDlg dialog


CProjectSettingsDlg::CProjectSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProjectSettingsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CProjectSettingsDlg)
	m_ProjectName = _T("");
	m_RunParams = _T("");
	m_RemotePath = _T("");
	m_WorkingDir = _T("");
	//}}AFX_DATA_INIT
}


void CProjectSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CProjectSettingsDlg)
	DDX_Text(pDX, IDC_PROJECTNAME_EDT, m_ProjectName);
	DDX_Text(pDX, IDC_RUNPARAM_EDT, m_RunParams);
	DDX_Text(pDX, IDC_REMPATH_EDT, m_RemotePath);
	DDX_Text(pDX, IDC_WORKDIR_EDT, m_WorkingDir);
	DDV_WorkingDir(pDX, m_WorkingDir);

	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CProjectSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CProjectSettingsDlg)
	ON_BN_CLICKED(IDC_BROWSEDIR_BTN, OnBrowseDirBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectSettingsDlg message handlers
void PASCAL CProjectSettingsDlg::DDV_WorkingDir(CDataExchange* pDX, CString workingDir)
{
	// check to make sure the path is not empty. if it is, we skip the check
	// otherwise, a dialog box will pop up when the settings are opened for the first time
/*	if (workingDir != "")
	{
		// check to see if the path we added is valid
		if (!(PathFileExists(workingDir)))
		{
			// if not, tell them to enter a valid path and not allow the dialog to close
			::AfxMessageBox("That path is not valid", MB_OK, 0);
			pDX->Fail();
		}
	}*/
}

void CProjectSettingsDlg::OnBrowseDirBtn() 
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
		m_WorkingDir = Path;

		UpdateData(FALSE);
	}
}
