//////////////////////////////////////////////////////////////////////////////
//  File:    EditorOptsPg.cpp
//
//  Purpose: This defines a single page of the options dialog - this page 
//  provides the interface for setting all editor options.
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
#include "dbgremotedoc.h"
#include "EditorOptsPg.h"
#include "MainFrm.h"

#include "scintilla\scintilla.h"
#include "scintilla\scilexer.h"

#include <set>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditorOptsPg property page

IMPLEMENT_DYNCREATE(CEditorOptsPg, CPropertyPage)

CEditorOptsPg::CEditorOptsPg() : CPropertyPage(CEditorOptsPg::IDD)
{
	//{{AFX_DATA_INIT(CEditorOptsPg)
	m_TabWidth = 4;
	m_bFoldOnOpen = FALSE;
	m_bUseTabs = TRUE;
	//}}AFX_DATA_INIT

}

CEditorOptsPg::~CEditorOptsPg()
{
}

void CEditorOptsPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditorOptsPg)
	DDX_Text(pDX, IDC_TABWIDTH_EDT, m_TabWidth);
	DDX_Check(pDX, IDC_FOLDOPEN_CHK, m_bFoldOnOpen);
	DDX_Check(pDX, IDC_USETABS_CHK, m_bUseTabs);
	//}}AFX_DATA_MAP
}



BOOL CEditorOptsPg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();

	UpdateLanguageList();
	CComboBox* pLangCbo = (CComboBox*)GetDlgItem(IDC_CURLANG_CBO);
	CListBox* pList = (CListBox*)GetDlgItem(IDC_STYLE_LST);

	int iSel = pLangCbo->GetCurSel();
	if (iSel < 0)
	{
		iSel = 0;
		pLangCbo->SetCurSel(iSel);
	}
	CString SelLang;
	pLangCbo->GetLBText(iSel, SelLang);

	UpdateStyleList(SelLang);

	pList->SetCurSel(0);
	const CUserCfg::SEditorStyle* pStyle = pCfg->GetEditorStyle(pList->GetItemData(0));
	if (pStyle)
	{
		memcpy(&m_CurrentStyle, pStyle, sizeof(m_CurrentStyle));
		ShowCurrentStyle();
	}

	m_TabWidth = pCfg->GetEditorTabWidth();
	m_CaretColor = pCfg->GetEditorCaretColor();
	m_bFoldOnOpen = pCfg->GetFoldOnOpen();
	m_bUseTabs = pCfg->GetEditorUseTabs();
	
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}




BEGIN_MESSAGE_MAP(CEditorOptsPg, CPropertyPage)
	//{{AFX_MSG_MAP(CEditorOptsPg)
	ON_BN_CLICKED(IDC_FORECOL_BTN, OnForecolBtn)
	ON_BN_CLICKED(IDC_FONTSEL_BTN, OnFontSelectBtn)
	ON_BN_CLICKED(IDC_BACKCOL_BTN, OnBackcolBtn)
	ON_WM_DRAWITEM()
	ON_LBN_SELCHANGE(IDC_STYLE_LST, OnSelchangeStyleLst)
	ON_BN_CLICKED(IDC_RESTORECUR_BTN, OnRestoreCurBtn)
	ON_BN_CLICKED(IDC_RESTOREALL_BTN, OnRestoreAllBtn)
	ON_BN_CLICKED(IDC_CARETCOL_BTN, OnCaretColorBtn)
	ON_EN_CHANGE(IDC_TABWIDTH_EDT, OnChangeTabwidthEdt)
	ON_CBN_SELCHANGE(IDC_CURLANG_CBO, OnSelchangeCurlangCbo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditorOptsPg message handlers

void CEditorOptsPg::OnForecolBtn() 
{
	CColorDialog dlg(m_CurrentStyle.ForeCol, CC_SOLIDCOLOR, this);
	if (dlg.DoModal() == IDOK)
	{
		m_CurrentStyle.ForeCol = dlg.GetColor();

		m_CurForeBrush.DeleteObject();
		m_CurForeBrush.CreateSolidBrush(dlg.GetColor());	
		GetDlgItem(IDC_FONTSEL_BTN)->Invalidate(TRUE);
		GetDlgItem(IDC_FORECOL_BTN)->Invalidate(TRUE);
		
		if (STYLE_DEFAULT == m_CurrentStyle.StyleId)
			SaveDefaultStyle(kForecol, "foreground color");
		SetModified(TRUE);
	}
}

void CEditorOptsPg::OnBackcolBtn() 
{
	CColorDialog dlg(m_CurrentStyle.BackCol, CC_SOLIDCOLOR, this);
	if (dlg.DoModal() == IDOK)
	{
		m_CurrentStyle.BackCol = dlg.GetColor();

		m_CurBackBrush.DeleteObject();
		m_CurBackBrush.CreateSolidBrush(dlg.GetColor());	
		GetDlgItem(IDC_FONTSEL_BTN)->Invalidate(TRUE);
		GetDlgItem(IDC_BACKCOL_BTN)->Invalidate(TRUE);

		if (STYLE_DEFAULT == m_CurrentStyle.StyleId)
			SaveDefaultStyle(kBackcol, "background color");
		SetModified(TRUE);
	}
}

void CEditorOptsPg::SaveDefaultStyle(EStyleTypes changedStyle, const char* description)
{
	string question = stringprintf("Would you like to apply the default %s to all styles?", description);
	if (IDYES == AfxMessageBox(question.c_str(), MB_YESNO|MB_ICONQUESTION) )
	{
		CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
		
		CUserCfg::SEditorStyle EditStyle = m_CurrentStyle;
		const CUserCfg::SEditorStyle* pStyle = 0;
		int i = 0;
		do
		{
			pStyle = pCfg->GetEditorStyle(i++);
			if (pStyle && (strcmp(pStyle->Languagename, m_CurrentStyle.Languagename) == 0))
			{
				CUserCfg::SEditorStyle mergedStyle = *pStyle;
				switch (changedStyle)
				{
					case kForecol:
						mergedStyle.ForeCol = EditStyle.ForeCol;
						break;

					case kBackcol:
						mergedStyle.BackCol = EditStyle.BackCol;
						break;

					case kFont:
						strcpy(mergedStyle.Facename, EditStyle.Facename);
						mergedStyle.PointSize = EditStyle.PointSize;
						mergedStyle.Bold = EditStyle.Bold;
						mergedStyle.Italic = EditStyle.Italic;
						mergedStyle.Underline = EditStyle.Underline;
						break;
				}
				pCfg->SetEditorStyle(mergedStyle.Languagename, mergedStyle.StyleId, &mergedStyle);
			}		
		}while (pStyle);
	}
}


void CEditorOptsPg::OnCaretColorBtn() 
{
	CColorDialog dlg(m_CaretColor, CC_SOLIDCOLOR, this);
	if (dlg.DoModal())
	{
		m_CaretColor = dlg.GetColor();
		SetModified(TRUE);
		GetDlgItem(IDC_CARETCOL_BTN)->Invalidate(TRUE);
		SetModified(TRUE);
	}
}


void CEditorOptsPg::OnFontSelectBtn() 
{
	LOGFONT lf;
	m_CurFont.GetLogFont(&lf);

	CFontDialog dlg(&lf, CF_INITTOLOGFONTSTRUCT|CF_SCREENFONTS|CF_NOSCRIPTSEL, NULL, this);
	if (dlg.DoModal() == IDOK)
	{
		strncpy(m_CurrentStyle.Facename, dlg.GetFaceName(), LF_FACESIZE);
		m_CurrentStyle.PointSize = dlg.GetSize() / 10;
		m_CurrentStyle.Bold = (dlg.IsBold() == TRUE);
		m_CurrentStyle.Italic = (dlg.IsItalic() == TRUE);
		m_CurrentStyle.Underline = (dlg.IsUnderline() == TRUE);
		
		ShowCurrentStyle();

		if (STYLE_DEFAULT == m_CurrentStyle.StyleId)
			SaveDefaultStyle(kFont, "font");
		SetModified(TRUE);
	}
}



void CEditorOptsPg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct) 
{	
	bool bDisplay = false;
	if ((nIDCtl == IDC_FORECOL_BTN) || (nIDCtl == IDC_BACKCOL_BTN) || 
		(nIDCtl == IDC_FONTSEL_BTN) || (nIDCtl == IDC_CARETCOL_BTN))
	{
		SetTextColor(lpDrawItemStruct->hDC, m_CurrentStyle.ForeCol);
		SetBkColor(lpDrawItemStruct->hDC, m_CurrentStyle.BackCol);

		RECT r;
		r = lpDrawItemStruct->rcItem;
		
		::FillRect(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, (HBRUSH) (COLOR_BTNFACE+1));

		InflateRect(&r, -4, -4);
		
		if (lpDrawItemStruct->itemState & ODS_SELECTED)
		{
			OffsetRect(&r, 2, 2);
			
			::DrawEdge(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, EDGE_SUNKEN, BF_RECT);
		}
		else
		{
			::DrawEdge(lpDrawItemStruct->hDC, &lpDrawItemStruct->rcItem, EDGE_RAISED, BF_RECT);
		}

		
		if (nIDCtl == IDC_FORECOL_BTN)
		{
			::FillRect(lpDrawItemStruct->hDC, &r, (HBRUSH)m_CurForeBrush);
		}
		else if (nIDCtl == IDC_BACKCOL_BTN)
		{
			::FillRect(lpDrawItemStruct->hDC, &r, (HBRUSH)m_CurBackBrush);
		}
		else if (nIDCtl == IDC_CARETCOL_BTN)
		{
			CBrush CaretBrush;
			CaretBrush.CreateSolidBrush(m_CaretColor);
			::FillRect(lpDrawItemStruct->hDC, &r, (HBRUSH)CaretBrush);
		}
		else if (nIDCtl == IDC_FONTSEL_BTN)
		{
			//output the text
			
			CString Txt;
			GetDlgItem(IDC_FONTSEL_BTN)->GetWindowText(Txt);
			DrawTextEx(
				  lpDrawItemStruct->hDC,                     // handle to DC
				  (char*)((const char*)Txt),             // text to draw
				  Txt.GetLength(),                 // length of text to draw
				  &r,
				  DT_VCENTER|DT_SINGLELINE|DT_CENTER|DT_WORD_ELLIPSIS, NULL);
		}
		
		return;
	}
	CPropertyPage::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CEditorOptsPg::OnSelchangeStyleLst() 
{
	SaveCurrentStyle();
	
	CListBox* pList = (CListBox*)GetDlgItem(IDC_STYLE_LST);	
	if (pList->GetCount() == 0)
		return;

	int CurSel = pList->GetCurSel();
	if (CurSel < 0)
	{
		CurSel = 0;
	}
	
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	const CUserCfg::SEditorStyle* pStyle = pCfg->GetEditorStyle(pList->GetItemData(CurSel));
	if (pStyle)
	{
		memcpy(&m_CurrentStyle, pStyle, sizeof(m_CurrentStyle));
		ShowCurrentStyle();
	}
	else
	{
		assert(0);
	}
}


void CEditorOptsPg::ShowCurrentStyle()
{
	m_CurForeBrush.DeleteObject();
	m_CurForeBrush.CreateSolidBrush(m_CurrentStyle.ForeCol);
	m_CurBackBrush.DeleteObject();
	m_CurBackBrush.CreateSolidBrush(m_CurrentStyle.BackCol);

	m_CurFont.DeleteObject();
	m_CurFont.CreatePointFont(m_CurrentStyle.PointSize*10, m_CurrentStyle.Facename);
	
	if (m_CurrentStyle.Bold || m_CurrentStyle.Italic || m_CurrentStyle.Underline)
	{
		LOGFONT lf;
		m_CurFont.GetLogFont(&lf);
		
		if (m_CurrentStyle.Bold)
			lf.lfWeight = FW_BOLD;

		if (m_CurrentStyle.Italic)
			lf.lfItalic = true;

		if (m_CurrentStyle.Underline)
			lf.lfUnderline = true;

		m_CurFont.DeleteObject();
		m_CurFont.CreateFontIndirect(&lf);
	}
	
	UpdateFontDisplay();

	GetDlgItem(IDC_FONTSEL_BTN)->Invalidate(TRUE);
	GetDlgItem(IDC_BACKCOL_BTN)->Invalidate(TRUE);
	GetDlgItem(IDC_FORECOL_BTN)->Invalidate(TRUE);

}


void CEditorOptsPg::UpdateFontDisplay()
{
	CString Temp = m_CurrentStyle.Facename;
	char buff[12];
	sprintf(buff, " %d point", m_CurrentStyle.PointSize);
	Temp += buff;
	if (m_CurrentStyle.Bold)
		Temp += " Bold";
	if (m_CurrentStyle.Italic)
		Temp += " Italic";
	if (m_CurrentStyle.Underline)
		Temp += " Under";

	GetDlgItem(IDC_FONTSEL_BTN)->SendMessage(WM_SETFONT, (WPARAM)((HFONT)m_CurFont), 1);
	GetDlgItem(IDC_FONTSEL_BTN)->SetWindowText(Temp);
}

void CEditorOptsPg::SaveCurrentStyle()
{
	UpdateData(TRUE);
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	pCfg->SetEditorStyle (m_CurrentStyle.Languagename, m_CurrentStyle.StyleId, &m_CurrentStyle);
	pCfg->SetEditorTabWidth(m_TabWidth);
	pCfg->SetEditorCaretColor(m_CaretColor);
	pCfg->SetFoldOnOpen((m_bFoldOnOpen == TRUE));
	pCfg->SetEditorUseTabs(m_bUseTabs == TRUE);
	CancelToClose();
	SetModified(TRUE);
}

void CEditorOptsPg::OnOK() 
{
	SaveCurrentStyle();

	UpdateViews();
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	pCfg->SaveUserCfg(); 

	CPropertyPage::OnOK();
}

BOOL CEditorOptsPg::OnApply() 
{
	SaveCurrentStyle();

	UpdateViews();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CEditorOptsPg::UpdateViews()
{
	CWinApp* pApp = AfxGetApp();
	assert(pApp);
	POSITION TemplPos = pApp->GetFirstDocTemplatePosition();
	while (TemplPos)
	{
		CDocTemplate* pTemplate = pApp->GetNextDocTemplate(TemplPos);
		assert(pTemplate);

		POSITION DocPos = pTemplate->GetFirstDocPosition();
		while (DocPos)
		{
			CDocument* pDoc = pTemplate->GetNextDoc(DocPos);
			if (pDoc)
			{
				pDoc->UpdateAllViews(0, CDbgRemoteDoc::HINT_UPDATESTYLE);
			}
		}
	}

	((CMainFrame*)AfxGetMainWnd())->pGetTraceViewBar()->UpdateStyles();
	((CMainFrame*)AfxGetMainWnd())->pGetStackViewBar()->UpdateStyles();
	((CMainFrame*)AfxGetMainWnd())->pGetProjectViewBar()->UpdateStyles();
}

void CEditorOptsPg::OnRestoreCurBtn() 
{
	AfxMessageBox("Not yet implemented.");
}

void CEditorOptsPg::OnRestoreAllBtn() 
{
	AfxMessageBox("Not yet implemented.");
}



void CEditorOptsPg::OnChangeTabwidthEdt() 
{
	SetModified(TRUE);
}

void CEditorOptsPg::UpdateLanguageList()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();

	set<string> setLanguageNames;
	CListBox* pList = (CListBox*)GetDlgItem(IDC_STYLE_LST);
	int i = 0;
	while(1)
	{
		const CUserCfg::SEditorStyle* pStyle = pCfg->GetEditorStyle(i++);
		if (pStyle)
		{		
			setLanguageNames.insert(pStyle->Languagename);
		}
		else
			break;
	}

	CComboBox* pLangCbo = (CComboBox*)GetDlgItem(IDC_CURLANG_CBO);
	assert(pLangCbo);
	pLangCbo->ResetContent();
	for (set<string>::iterator itrLangName = setLanguageNames.begin(); itrLangName != setLanguageNames.end(); itrLangName++)
	{
		pLangCbo->AddString(itrLangName->c_str());
	}
	pLangCbo->SetCurSel(pCfg->GetLastDocOptsType());
}

void CEditorOptsPg::UpdateStyleList(const char* cszLanguageName)
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	CListBox* pList = (CListBox*)GetDlgItem(IDC_STYLE_LST);
	pList->ResetContent();
	int i = 0;
	while(1)
	{
		const CUserCfg::SEditorStyle* pStyle = pCfg->GetEditorStyle(i);
		if (pStyle)
		{		
			if (cszLanguageName && strlen(cszLanguageName))
			{
				//if a language name is specified, filter out any that don't match
				if (strcmp(cszLanguageName, pStyle->Languagename) != 0)
				{
					++i;
					continue;
				}
			}
			int nInx = pList->AddString(pStyle->StyleName);
			//store the style index in the item data
			pList->SetItemData(nInx, i);
		}
		else
			break;
		
		++i;
	}
}

void CEditorOptsPg::OnSelchangeCurlangCbo() 
{
	CComboBox* pLangCbo = (CComboBox*)GetDlgItem(IDC_CURLANG_CBO);
	CListBox* pList = (CListBox*)GetDlgItem(IDC_STYLE_LST);
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	int iSel = pLangCbo->GetCurSel();
	if (iSel < 0)
	{
		iSel = 0;
		pLangCbo->SetCurSel(iSel);
	}

	pCfg->SetLastDocOptsType(iSel);

	CString SelLang;
	pLangCbo->GetLBText(iSel, SelLang);

	UpdateStyleList(SelLang);

	pList->SetCurSel(0);
	const CUserCfg::SEditorStyle* pStyle = pCfg->GetEditorStyle(pList->GetItemData(0));
	if (pStyle)
	{
		memcpy(&m_CurrentStyle, pStyle, sizeof(m_CurrentStyle));
		ShowCurrentStyle();
	}
}
