//////////////////////////////////////////////////////////////////////////////
//  File:    ConnectSettingsDlg.cpp
//
//  Purpose: A dialog box that allows the user to specify connection settings
//  for the connection to the remote debug engine.
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

// ConnectSettingsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DbgRemote.h"
#include "ConnectSettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectSettingsDlg dialog


CConnectSettingsDlg::CConnectSettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectSettingsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConnectSettingsDlg)
	m_RemoteName = _T("");
	m_DebugScript = _T("");
	//}}AFX_DATA_INIT
	m_pConnectNames = NULL;
	m_nConnectType = 0;
}


void CConnectSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectSettingsDlg)
	DDX_CBString(pDX, IDC_REMOTENAME_CBO, m_RemoteName);
	DDX_Text(pDX, IDC_DEBUGNM_STC, m_DebugScript);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectSettingsDlg, CDialog)
	//{{AFX_MSG_MAP(CConnectSettingsDlg)
	ON_BN_CLICKED(IDC_LOCAL_RDO, OnLocalRdo)
	ON_BN_CLICKED(IDC_REMOTE_RDO, OnRemoteRdo)
	ON_BN_CLICKED(IDC_SETDBGSCRIPT_BTN, OnSetdbgscriptBtn)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectSettingsDlg message handlers

void CConnectSettingsDlg::OnLocalRdo() 
{
	m_nConnectType = 0;
	GetDlgItem(IDC_REMOTENAME_STC)->EnableWindow(FALSE);
	GetDlgItem(IDC_REMOTENAME_CBO)->EnableWindow(FALSE);
}

void CConnectSettingsDlg::OnRemoteRdo() 
{
	m_nConnectType = 1;
	GetDlgItem(IDC_REMOTENAME_STC)->EnableWindow(m_nConnectType == 1);
	GetDlgItem(IDC_REMOTENAME_CBO)->EnableWindow(m_nConnectType == 1);
}

void CConnectSettingsDlg::OnOK() 
{
	UpdateData(TRUE);	
	CDialog::OnOK();
}

BOOL CConnectSettingsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (m_pConnectNames && m_pConnectNames->size())
	{
		CComboBox* pNamesCbo = (CComboBox*)GetDlgItem(IDC_REMOTENAME_CBO);
		
		for (unsigned int i = 0; i < m_pConnectNames->size(); i++)
		{	
			pNamesCbo->AddString(m_pConnectNames->at(i).c_str());
			m_RemoteName = m_pConnectNames->at(i).c_str();
		}
		pNamesCbo->SetWindowText(m_pConnectNames->at(0).c_str());
		pNamesCbo->SelectString(0, m_pConnectNames->at(0).c_str());
	}

	if (m_nConnectType == 0)
	{
		CheckRadioButton(IDC_LOCAL_RDO, IDC_REMOTE_RDO, IDC_LOCAL_RDO);
	}
	else
	{
		CheckRadioButton(IDC_LOCAL_RDO, IDC_REMOTE_RDO, IDC_REMOTE_RDO);
	}
	GetDlgItem(IDC_REMOTENAME_STC)->EnableWindow(m_nConnectType == 1);
	GetDlgItem(IDC_REMOTENAME_CBO)->EnableWindow(m_nConnectType == 1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConnectSettingsDlg::OnSetdbgscriptBtn() 
{
	CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
	assert(pApp);

	pApp->OnDebugRunOptions();
	m_DebugScript = pApp->GetDebugScript();
}
