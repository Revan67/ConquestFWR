// DebugOptionsPage.cpp : implementation file

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
#include "DebugOptionsPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDebugOptionsPage property page

IMPLEMENT_DYNCREATE(CDebugOptionsPage, CPropertyPage)

CDebugOptionsPage::CDebugOptionsPage() : CPropertyPage(CDebugOptionsPage::IDD)
, m_bClearOutput(FALSE)
{
	//{{AFX_DATA_INIT(CDebugOptionsPage)
	m_EnableAdvancedDebug = FALSE;
	m_BreakOnFirstLine = FALSE;
	//}}AFX_DATA_INIT
}

CDebugOptionsPage::~CDebugOptionsPage()
{
}

void CDebugOptionsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDebugOptionsPage)
	DDX_Check(pDX, IDC_ADVDEBUG_CHK, m_EnableAdvancedDebug);
	DDX_Check(pDX, IDC_BREAKFIRSTLINE_CHK, m_BreakOnFirstLine);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CLEAROUT_CHK, m_bClearOutput);
}


BEGIN_MESSAGE_MAP(CDebugOptionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CDebugOptionsPage)
	ON_BN_CLICKED(IDC_BRAKUNHANDLED_RDO, OnBrakUnhandledRdo)
	ON_BN_CLICKED(IDC_BREAKALWAYS_RDO, OnBreakAlwaysRdo)
	ON_BN_CLICKED(IDC_IGNOREEXCEP_RDO, OnIgnoreExcepRdo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDebugOptionsPage message handlers

BOOL CDebugOptionsPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	m_EnableAdvancedDebug = pCfg->GetAdvancedDebugEnabled();
	m_BreakOnFirstLine = pCfg->GetBreakOnFirstLine();
	m_bClearOutput = pCfg->GetClearOutput();
	m_eExceptionMode = pCfg->GetExceptionMode();
	switch(m_eExceptionMode)
	{
		case CUserCfg::eEXCEPTMODE_BREAKALL:
			CheckRadioButton(IDC_BREAKALWAYS_RDO, IDC_IGNOREEXCEP_RDO, IDC_BREAKALWAYS_RDO);
			break;
		case CUserCfg::eEXCEPTMODE_BREAKUNHAND:
			CheckRadioButton(IDC_BREAKALWAYS_RDO, IDC_IGNOREEXCEP_RDO, IDC_BRAKUNHANDLED_RDO);
			break;
		case CUserCfg::eEXCEPTMODE_IGNORE:
			CheckRadioButton(IDC_BREAKALWAYS_RDO, IDC_IGNOREEXCEP_RDO, IDC_IGNOREEXCEP_RDO);
			break;

		default:
			assert(0);
	}
	
		
		
	
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CDebugOptionsPage::OnApply() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	pCfg->SetAdvancedDebugEnabled(m_EnableAdvancedDebug == TRUE);
	pCfg->SetBreakOnFirstLine(m_BreakOnFirstLine == TRUE);
	pCfg->SetExceptionMode(m_eExceptionMode);
	pCfg->SetClearOutput(m_bClearOutput == TRUE);
	
	
	return CPropertyPage::OnApply();
}

void CDebugOptionsPage::OnOK() 
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	pCfg->SetAdvancedDebugEnabled(m_EnableAdvancedDebug == TRUE);
	pCfg->SetBreakOnFirstLine(m_BreakOnFirstLine == TRUE);
	pCfg->SetExceptionMode(m_eExceptionMode);
	pCfg->SetClearOutput(m_bClearOutput == TRUE);
	
	CPropertyPage::OnOK();
}

void CDebugOptionsPage::OnBrakUnhandledRdo() 
{
	m_eExceptionMode = CUserCfg::eEXCEPTMODE_BREAKUNHAND;
}

void CDebugOptionsPage::OnBreakAlwaysRdo() 
{
	m_eExceptionMode = CUserCfg::eEXCEPTMODE_BREAKALL;
}

void CDebugOptionsPage::OnIgnoreExcepRdo() 
{
	m_eExceptionMode = CUserCfg::eEXCEPTMODE_IGNORE;
}
