//////////////////////////////////////////////////////////////////////////////
//  File:    TextFindDlg.cpp
//
//  Purpose: This is a dialog box which allows you to define parameters for
//  searching through an open python file - the actual searching is done by
//  the scintilla control - this dialog simply allows you to define what and 
//  how.
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
#include "TextFindDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextFindDlg dialog


CTextFindDlg::CTextFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTextFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTextFindDlg)
	m_FindText = _T("");
	m_MatchCase = FALSE;
	m_MatchStart = FALSE;
	m_MatchWord = FALSE;
	m_UseRegExpr = FALSE;
	//}}AFX_DATA_INIT
}


void CTextFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextFindDlg)
	DDX_CBString(pDX, IDC_FINDTXT_CBO, m_FindText);
	DDX_Check(pDX, IDC_MATCHCASE_CHK, m_MatchCase);
	DDX_Check(pDX, IDC_MATCHSTART_CHK, m_MatchStart);
	DDX_Check(pDX, IDC_MATCHWORD_CHK, m_MatchWord);
	DDX_Check(pDX, IDC_REGEXP_CHK, m_UseRegExpr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextFindDlg, CDialog)
	//{{AFX_MSG_MAP(CTextFindDlg)
	ON_BN_CLICKED(IDC_REGEX_BTN, OnRegexBtn)
	ON_COMMAND(ID_FINDPOP_ANYCHAR, OnFindpopAnychar)
	ON_COMMAND(ID_FINDPOP_CHARINSET, OnFindpopCharinset)
	ON_COMMAND(ID_FINDPOP_CHARNOTINSET, OnFindpopCharnotinset)
	ON_COMMAND(ID_FINDPOP_ENDOFLINE, OnFindpopEndofline)
	ON_COMMAND(ID_FINDPOP_ENDWORD, OnFindpopEndword)
	ON_COMMAND(ID_FINDPOP_ONEMORE, OnFindpopOnemore)
	ON_COMMAND(ID_FINDPOP_STARTOFLINE, OnFindpopStartofline)
	ON_COMMAND(ID_FINDPOP_STARTWORD, OnFindpopStartword)
	ON_COMMAND(ID_FINDPOP_TABCHAR, OnFindpopTabchar)
	ON_COMMAND(ID_FINDPOP_TAGPART, OnFindpopTagpart)
	ON_COMMAND(ID_FINDPOP_ZEROMORE, OnFindpopZeromore)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextFindDlg message handlers

BOOL CTextFindDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();

	CComboBox* pCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	const std::vector<string>* pFindHist = pCfg->GetTextFindWhat();
	for (std::vector<string>::size_type i = 0; i < pFindHist->size(); i++)
	{
		pCbo->InsertString(0,(*pFindHist)[i].c_str());
		pCbo->SelectString(0, (*pFindHist)[i].c_str());
	}
	
	
	UpdateData(FALSE);
	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTextFindDlg::OnOK() 
{
	UpdateData(TRUE);
	if (m_FindText.GetLength())
	{
		CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
		pCfg->AddTextFindWhat(m_FindText);
	}
	CDialog::OnOK();
}

void CTextFindDlg::OnRegexBtn() 
{
	CComboBox* pFindCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	DWORD dwSel = pFindCbo->GetEditSel();


    CWnd* pRegexBtn = GetDlgItem(IDC_REGEX_BTN);
	if (pRegexBtn)
	{
		CRect r;
		pRegexBtn->GetWindowRect(&r);

		CMenu menu;
		if (menu.LoadMenu(IDR_FINDPOPUP))
		{
			CMenu* pPopup = menu.GetSubMenu(0);
			ASSERT(pPopup != NULL);

			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON,
			                       r.left, r.top, this);
		}
	}
}

BOOL CTextFindDlg::Create(CWnd* pParentWnd) 
{
	return CDialog::Create(IDD, pParentWnd);
}



void CTextFindDlg::InsertFindText(const char* cszInsert, int nSelect)
{
	CComboBox* pFindCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	DWORD dwSel = pFindCbo->GetEditSel();

	UpdateData(TRUE);

	DWORD dwSelStart = LOWORD(dwSel);
	DWORD dwSelEnd   = HIWORD(dwSel);
	if (dwSelStart != dwSelEnd)
	{
		m_FindText.Delete(_MIN(dwSelStart, dwSelEnd), abs(dwSelStart - dwSelEnd) + 1);
		dwSelStart  = _MIN(dwSelStart, dwSelEnd);
	}

	m_FindText.Insert(dwSelStart, cszInsert);
	UpdateData(FALSE);
	int nNewSelect = dwSelStart + nSelect;

	if (nSelect < 0)
	{
		nNewSelect = dwSelStart + strlen(cszInsert);
	}
	pFindCbo->SetEditSel(nNewSelect, nNewSelect);
	
}

	/*	Special characters interpreted are 
		'.' for any character, 
		'\(' and '\)' to tag parts of the match, 
		'\1' .. '\9' to refer to tagged parts, 
		'\<' to match the start of a word, 
		'\>' to match the end of a word, 
		'\' as a quote character, 
		'[' and ']' to indicate a set of characters, 
		^ within a set to complement the set, 
		^ outside a set to match the start of line, 
		$ to match the end of line, 
		* to match 0 or more times, 
		+ to match 1 or more times 
	*/
void CTextFindDlg::OnFindpopAnychar() 
{
	InsertFindText(".");
}

void CTextFindDlg::OnFindpopCharinset() 
{
	InsertFindText("[]", 1);	
}

void CTextFindDlg::OnFindpopCharnotinset() 
{
	InsertFindText("[^]", 2);
}

void CTextFindDlg::OnFindpopEndofline() 
{
	InsertFindText("$");
}

void CTextFindDlg::OnFindpopEndword() 
{
	InsertFindText("\\>");
}

void CTextFindDlg::OnFindpopOnemore() 
{
	InsertFindText("+");
}

void CTextFindDlg::OnFindpopStartofline() 
{
	InsertFindText("^");
}

void CTextFindDlg::OnFindpopStartword() 
{
	InsertFindText("\\<");
}

void CTextFindDlg::OnFindpopTabchar() 
{
	InsertFindText("\\t");
}

void CTextFindDlg::OnFindpopTagpart() 
{
	InsertFindText("\\(\\)", 2);
}

void CTextFindDlg::OnFindpopZeromore() 
{
	InsertFindText("*");
}




