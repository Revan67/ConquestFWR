//////////////////////////////////////////////////////////////////////////////
//  File:    TextReplaceDlg.cpp
//
//  Purpose: This is a dialog box that allows you to do a search and replace on 
//  an opened python file.  The actual searching is done by the scintilla control.
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

// TextReplaceDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "TextReplaceDlg.h"
#include "scintilla\scintilla.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextReplaceDlg dialog


CTextReplaceDlg::CTextReplaceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTextReplaceDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTextReplaceDlg)
	m_strReplaceWhat = _T("");
	m_strReplaceWith = _T("");
	m_bMatchCase = FALSE;
	m_bMatchStart = FALSE;
	m_bMatchWord = FALSE;
	m_bRegExpression = FALSE;
	//}}AFX_DATA_INIT
	m_bEnableReplaceSel = TRUE;
	m_bReplaceSel = FALSE;
	m_hScintillaHwnd = 0;
}


void CTextReplaceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextReplaceDlg)
	DDX_CBString(pDX, IDC_REPLACEWHAT_CBO, m_strReplaceWhat);
	DDX_CBString(pDX, IDC_REPLACEWITH_CBO, m_strReplaceWith);
	DDX_Check(pDX, IDC_MATCHCASE_CHK, m_bMatchCase);
	DDX_Check(pDX, IDC_MATCHSTART_CHK, m_bMatchStart);
	DDX_Check(pDX, IDC_MATCHWORD_CHK, m_bMatchWord);
	DDX_Check(pDX, IDC_REGEXP_CHK, m_bRegExpression);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextReplaceDlg, CDialog)
	//{{AFX_MSG_MAP(CTextReplaceDlg)
	ON_BN_CLICKED(IDC_REPLACESEL_RDO, OnReplaceSelectedRdo)
	ON_BN_CLICKED(IDC_FINDNEXT_BTN, OnFindNextBtn)
	ON_BN_CLICKED(IDC_REPLACE_BTN, OnReplaceBtn)
	ON_BN_CLICKED(IDC_REPLACEALL_BTN, OnReplaceAllBtn)
	ON_BN_CLICKED(IDC_REPWHOLE_RDO, OnReplaceInWholeFileRdo)
	ON_WM_CREATE()
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
	ON_BN_CLICKED(IDC_REGEXP_CHK, OnRegexpChk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextReplaceDlg message handlers

BOOL CTextReplaceDlg::Create(CWnd* pParentWnd) 
{
	return CDialog::Create(IDD, pParentWnd);
}

int CTextReplaceDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	

	return 0;
}


BOOL CTextReplaceDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	assert(m_hScintillaHwnd);
	if (m_hScintillaHwnd)
	{
		int StartPos = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONSTART, 0, 0);
		int EndPos = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONEND, 0, 0);
		m_bEnableReplaceSel = (StartPos != EndPos);
		m_bReplaceSel = (StartPos != EndPos);
	}
	
	EnableReplaceSelected(m_bEnableReplaceSel);
	SetReplaceSelected(m_bReplaceSel);

	if (m_bReplaceSel)
	{
		m_nAbsEndPos = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONEND, 0, 0);
	}
	else
	{
		m_nAbsEndPos = ::SendMessage(m_hScintillaHwnd, SCI_GETLENGTH, 0, 0);
	}

	OnRegexpChk();

	UpdateHistory();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CTextReplaceDlg::SetReplaceSelected(BOOL bReplaceSel)
{
	m_bReplaceSel = bReplaceSel;
	if (bReplaceSel)
	{
		if (!m_bEnableReplaceSel)
			EnableReplaceSelected(TRUE);
		
		CheckRadioButton(IDC_REPLACESEL_RDO, IDC_REPWHOLE_RDO, IDC_REPLACESEL_RDO);
	}
	else
	{
		CheckRadioButton(IDC_REPLACESEL_RDO, IDC_REPWHOLE_RDO, IDC_REPWHOLE_RDO);
	}
}

void CTextReplaceDlg::EnableReplaceSelected(BOOL bEnable)
{
	m_bEnableReplaceSel = bEnable;
	if (m_hWnd && ::IsWindow(m_hWnd))
	{
		GetDlgItem(IDC_REPLACESEL_RDO)->EnableWindow(bEnable);
	}
}

void CTextReplaceDlg::OnReplaceSelectedRdo() 
{
	SetReplaceSelected(TRUE);
}

void CTextReplaceDlg::OnReplaceInWholeFileRdo() 
{
	SetReplaceSelected(FALSE);
}

void CTextReplaceDlg::OnFindNextBtn() 
{
	UpdateData (TRUE);
	FindNext();
}

void CTextReplaceDlg::OnReplaceBtn() 
{
	UpdateData(TRUE);
	ReplaceNext();
}

void CTextReplaceDlg::OnReplaceAllBtn() 
{
	UpdateData(TRUE);
	ReplaceAll();
}

bool CTextReplaceDlg::FindNext(bool bNotifyOnFail)
{
	//add the new find and replace strings to the history
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	if (m_strReplaceWhat.GetLength() == 0)
		return false;

	pCfg->AddTextFindWhat(m_strReplaceWhat);
	
	if (m_strReplaceWith.GetLength() > 0)
		pCfg->AddTextReplaceWith(m_strReplaceWith);

	//update the history combo box lists
	UpdateHistory();

	//set scintilla's search flags based on the current GUI checks
	SetSearchFlags();

	//start the search from the end of the current selection
	int PosStart = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONEND, 0, 0);
	::SendMessage(m_hScintillaHwnd, SCI_SETTARGETSTART, PosStart, 0);
	
	//end the search at the end position (either the end of the doc or the end of the 
	//initially selected area
	::SendMessage(m_hScintillaHwnd, SCI_SETTARGETEND, m_nAbsEndPos, 0);

	//find the text
	char* pSearchStr = (char*)((const char*)m_strReplaceWhat);
	int iFind = ::SendMessage(m_hScintillaHwnd, SCI_SEARCHINTARGET, strlen(pSearchStr), (LPARAM)pSearchStr);
	if  (iFind > -1)
	{
		//text found - update the 'target'
		int start = ::SendMessage(m_hScintillaHwnd, SCI_GETTARGETSTART, 0, 0);
		int end = ::SendMessage(m_hScintillaHwnd, SCI_GETTARGETEND, 0, 0);

	
		//make sure the newly selected lines are visible
		int lineStart = ::SendMessage(m_hScintillaHwnd, SCI_LINEFROMPOSITION, start, 0);
		int lineEnd = ::SendMessage(m_hScintillaHwnd, SCI_LINEFROMPOSITION, end, 0);
		for (int line = lineStart; line <= lineEnd; line++) 
		{
			::SendMessage(m_hScintillaHwnd, SCI_ENSUREVISIBLE, line, 0);
		}
		
		//set the selection
		::SendMessage(m_hScintillaHwnd, SCI_SETSEL, start, end);
		return true;
	}
	//show the user that the search is complete
	if (bNotifyOnFail)
		AfxMessageBox("Search Complete", MB_OK|MB_ICONINFORMATION);
	return false;

} // FindNext

bool CTextReplaceDlg::ReplaceNext()
{
	//set the selection to before the current selection (search from here on)
	int SelStart = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONSTART, 0, 0);
	::SendMessage(m_hScintillaHwnd, SCI_SETSEL, SelStart, SelStart);
	
	//find the text
	if (FindNext())
	{
		if (m_strReplaceWith.GetLength() == 0)
			m_strReplaceWith = "";

		//if (m_strReplaceWith.GetLength())
		{
			//get the text to replace
			int SelStart = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONSTART, 0, 0);
			int SelEnd = ::SendMessage(m_hScintillaHwnd, SCI_GETSELECTIONEND, 0, 0);
			int RepLength = m_strReplaceWith.GetLength();

			//replace the text
			::SendMessage(m_hScintillaHwnd, SCI_REPLACESEL, 0, (LPARAM)((const char*)m_strReplaceWith));
			
			//select the replaced text
			::SendMessage(m_hScintillaHwnd, SCI_SETSEL, SelStart + RepLength, SelStart + RepLength);
			
			//update the end replace marker
			m_nAbsEndPos += RepLength - (SelEnd - SelStart);

			//move to the next match (if any)
			return FindNext();
		}
		return true;
	}
	return false;
}

void CTextReplaceDlg::ReplaceAll()
{
	while (ReplaceNext());
}

void CTextReplaceDlg::SetSearchFlags()
{
	int SearchFlags = 0;
	if (m_bMatchCase)
		SearchFlags |= SCFIND_MATCHCASE;
	if (m_bMatchWord)
		SearchFlags |= SCFIND_WHOLEWORD;
	if (m_bMatchStart)
		SearchFlags |= SCFIND_WORDSTART;
	if (m_bRegExpression)
		SearchFlags |= SCFIND_REGEXP;

	::SendMessage(m_hScintillaHwnd, SCI_SETSEARCHFLAGS, SearchFlags, 0);
}

void CTextReplaceDlg::OnRegexBtn() 
{
	//display a popup menu over the button that lists regular expression
	//items that can be added into the find combo box
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

void CTextReplaceDlg::OnRegexpChk() 
{
	//when the user checks the regular expression button, enable
	//or disable the regex button (popup menu)
	UpdateData(TRUE);
	GetDlgItem(IDC_REGEX_BTN)->EnableWindow(m_bRegExpression);
}


void CTextReplaceDlg::InsertFindText(const char* cszInsert, int nSelect)
{
	//insert text into the selected position of the find combo box

	CComboBox* pFindCbo = (CComboBox*)GetDlgItem(IDC_REPLACEWHAT_CBO);
	DWORD dwSel = pFindCbo->GetEditSel();

	UpdateData(TRUE);

	DWORD dwSelStart = LOWORD(dwSel);
	DWORD dwSelEnd   = HIWORD(dwSel);
	if (dwSelStart != dwSelEnd)
	{
		m_strReplaceWhat.Delete(_MIN(dwSelStart, dwSelEnd), abs(dwSelStart - dwSelEnd) + 1);
		dwSelStart  = _MIN(dwSelStart, dwSelEnd);
	}

	m_strReplaceWhat.Insert(dwSelStart, cszInsert);
	UpdateData(FALSE);
	int nNewSelect = dwSelStart + nSelect;

	if (nSelect < 0)
	{
		nNewSelect = dwSelStart + strlen(cszInsert);
	}
	pFindCbo->SetEditSel(nNewSelect, nNewSelect);
	
}

//	Special characters interpreted are 
//	'.' for any character, 
//	'\(' and '\)' to tag parts of the match, 
//	'\1' .. '\9' to refer to tagged parts, 
//	'\<' to match the start of a word, 
//	'\>' to match the end of a word, 
//	'\' as a quote character, 
//	'[' and ']' to indicate a set of characters, 
//	^ within a set to complement the set, 
//	^ outside a set to match the start of line, 
//	$ to match the end of line, 
//	* to match 0 or more times, 
//	+ to match 1 or more times 

void CTextReplaceDlg::OnFindpopAnychar() 
{
	InsertFindText(".");
}

void CTextReplaceDlg::OnFindpopCharinset() 
{
	InsertFindText("[]", 1);	
}

void CTextReplaceDlg::OnFindpopCharnotinset() 
{
	InsertFindText("[^]", 2);
}

void CTextReplaceDlg::OnFindpopEndofline() 
{
	InsertFindText("$");
}

void CTextReplaceDlg::OnFindpopEndword() 
{
	InsertFindText("\\>");
}

void CTextReplaceDlg::OnFindpopOnemore() 
{
	InsertFindText("+");
}

void CTextReplaceDlg::OnFindpopStartofline() 
{
	InsertFindText("^");
}

void CTextReplaceDlg::OnFindpopStartword() 
{
	InsertFindText("\\<");
}

void CTextReplaceDlg::OnFindpopTabchar() 
{
	InsertFindText("\\t");
}

void CTextReplaceDlg::OnFindpopTagpart() 
{
	InsertFindText("\\(\\)", 2);
}

void CTextReplaceDlg::OnFindpopZeromore() 
{
	InsertFindText("*");
}


void CTextReplaceDlg::UpdateHistory()
{
	//update the combo boxes containing the history of finds and replaces
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	CComboBox* pCbo = (CComboBox*)GetDlgItem(IDC_REPLACEWHAT_CBO);
	const vector<string>* pFindWhat = pCfg->GetTextFindWhat();
	for (vector<string>::const_iterator pCur = pFindWhat->begin(); pCur != pFindWhat->end(); pCur++)
	{
		pCbo->InsertString(0,pCur->c_str());
		pCbo->SelectString(0, pCur->c_str());
	}
	
	pCbo = (CComboBox*)GetDlgItem(IDC_REPLACEWITH_CBO);
	const vector<string>* pReplaceW = pCfg->GetTextReplaceWith();
	for (vector<string>::const_iterator pCur = pReplaceW->begin(); pCur != pReplaceW->end(); pCur++)
	{
		pCbo->InsertString(0,pCur->c_str());
		pCbo->SelectString(0, pCur->c_str());
	}
}



