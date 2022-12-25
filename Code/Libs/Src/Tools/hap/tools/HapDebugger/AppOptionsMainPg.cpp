//////////////////////////////////////////////////////////////////////////////
//  File:    AppOptionsMainPg.cpp
//
//  Purpose: This file implements the app options page of the options tabbed
//  dialog box.
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

// AppOptionsMainPg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "AppOptionsMainPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAppOptionsMainPg property page

IMPLEMENT_DYNCREATE(CAppOptionsMainPg, CPropertyPage)

CAppOptionsMainPg::CAppOptionsMainPg() : CPropertyPage(CAppOptionsMainPg::IDD)
{
	//{{AFX_DATA_INIT(CAppOptionsMainPg)
	m_bAutoOpenProject = FALSE;
	m_PythonHelpFile = _T("");
	m_bOpenXmlExtern = FALSE;
	m_bAutowatchPopup = FALSE;
	//}}AFX_DATA_INIT
}

CAppOptionsMainPg::~CAppOptionsMainPg()
{
}

void CAppOptionsMainPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAppOptionsMainPg)
	DDX_Check(pDX, IDC_AUTOOPENPROJ_CHK, m_bAutoOpenProject);
	DDX_Text(pDX, IDC_PYTHONHELP_EDT, m_PythonHelpFile);
	DDX_Check(pDX, IDC_XMLEXTERN_CHK, m_bOpenXmlExtern);
	DDX_Check(pDX, IDC_AUTOWATCHPOP_CHK, m_bAutowatchPopup);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAppOptionsMainPg, CPropertyPage)
	//{{AFX_MSG_MAP(CAppOptionsMainPg)
	ON_BN_CLICKED(IDC_HELPBROWSE_BTN, OnHelpBrowseBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAppOptionsMainPg message handlers




BOOL CAppOptionsMainPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	m_bAutoOpenProject = pCfg->GetOpenLastProj();
	m_PythonHelpFile = pCfg->GetPythonHelpFile();
	m_bOpenXmlExtern = pCfg->GetOpenXmlFilesExternal();
	
	m_bAutowatchPopup = pCfg->GetAutoWatch();
	m_PythonHelpFile = pCfg->GetPythonHelpFile();


	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAppOptionsMainPg::OnOK() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	pCfg->SetOpenLastProj(m_bAutoOpenProject == TRUE);
	pCfg->SetPythonHelpFile(m_PythonHelpFile);
	pCfg->SetOpenXmlFilesExternal(m_bOpenXmlExtern == TRUE);

	pCfg->SetAutoWatch(m_bAutowatchPopup == TRUE);

	CPropertyPage::OnOK();
}

BOOL CAppOptionsMainPg::OnApply() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	pCfg->SetOpenLastProj(m_bAutoOpenProject == TRUE);
	pCfg->SetPythonHelpFile(m_PythonHelpFile);
	pCfg->SetOpenXmlFilesExternal(m_bOpenXmlExtern == TRUE);
	pCfg->SetAutoWatch(m_bAutowatchPopup == TRUE);

	return CPropertyPage::OnApply();
}

void CAppOptionsMainPg::OnHelpBrowseBtn() 
{
	UpdateData(TRUE);

	CFileDialog dlg(TRUE, "*.chm", m_PythonHelpFile, 
		OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON, 
		"Compiled Help Files (*.chm)|*.chm||", this);	

	if (dlg.DoModal() == IDOK)
	{
		m_PythonHelpFile = dlg.GetPathName();
		UpdateData(FALSE);
	}
}
