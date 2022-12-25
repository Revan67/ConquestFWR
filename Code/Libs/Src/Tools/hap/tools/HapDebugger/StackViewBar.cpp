//////////////////////////////////////////////////////////////////////////////
//  File:    StackViewBar.cpp
//
//  Purpose: This is a dockable window which displays the call stack
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

// StackViewBar.cpp : implementation file
//

#include "stdafx.h"
#include "DbgRemote.h"
#include "StackViewBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



#define STACK_LB_IDENT	1010

/////////////////////////////////////////////////////////////////////////////
// CStackViewBar

CStackViewBar::CStackViewBar()
{
}

CStackViewBar::~CStackViewBar()
{
}


BEGIN_MESSAGE_MAP(CStackViewBar, CSizingControlBar)
	//{{AFX_MSG_MAP(CStackViewBar)
	ON_WM_CREATE()
    ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
	ON_LBN_DBLCLK(STACK_LB_IDENT, OnDblclickStackList)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CStackViewBar message handlers
int CStackViewBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
    if (CSizingControlBar::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (! m_List.Create(WS_CHILD|WS_VISIBLE|WS_BORDER|LBS_NOTIFY|LBS_MULTICOLUMN|WS_VSCROLL|WS_HSCROLL, CRect(0,0,10,10), this, STACK_LB_IDENT) )
		return -1;
	
	m_hBkBrush = 0;

	UpdateStyles();
    return 0;
}

void CStackViewBar::UpdateStyles()
{
	CUserCfg* pCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	CUserCfg::SEditorStyle stackStyle;
	pCfg->GetEditorStyle(kGeneralStyles, HAPSTYLE_STACKVW, &stackStyle);			
	m_ForeColor = stackStyle.ForeCol;
	m_BackColor = stackStyle.BackCol;
		
	m_Font.DeleteObject();
	if (!m_Font.CreatePointFont(stackStyle.PointSize * 10, stackStyle.Facename))
        return;
    m_List.SetFont(&m_Font);
	if (m_hBkBrush)
		::DeleteObject(m_hBkBrush);
	m_hBkBrush = ::CreateSolidBrush(stackStyle.BackCol);

	Invalidate(TRUE);
}

void CStackViewBar::OnSize(UINT nType, int cx, int cy) 
{
    CSizingControlBar::OnSize(nType, cx, cy);

    CRect rc;
    GetClientRect(rc);

    m_List.MoveWindow(rc, FALSE);
	m_List.SetColumnWidth(rc.Width() * 2);
}

HBRUSH CStackViewBar::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	if (nCtlColor == CTLCOLOR_LISTBOX   )
	{
		pDC->SetBkColor(m_BackColor); // change the background color
		pDC->SetTextColor(m_ForeColor); // change the text color

		return m_hBkBrush;
	}
	
	return CSizingControlBar::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CStackViewBar::OnClose() 
{
	DeleteObject(m_hBkBrush);
	CSizingControlBar::OnClose();
}


void CStackViewBar::SetCallStack(char_vector& vecCallStack)
{
	m_vecCallStack = vecCallStack;

	const char* pCurIn = &(vecCallStack[0]);
	int CurLen = strlen(pCurIn);
	m_List.ResetContent();

	while (CurLen > 0)
	{
		//pCurIn is in the form "pathfile,line, func(funcp1, funcp2, ..)"
		
		CString CurStr = pCurIn;
		FormatStackEntry(CurStr);
		m_List.AddString(CurStr);
		
		pCurIn += CurLen + 1;
		CurLen = strlen(pCurIn);
	}
}


void CStackViewBar::FormatStackEntry(CString& StackEntry)
{
	//convert "pathfile,line, func(funcp1, funcp2, ..)"
	//into "file(line), funct(p1, p2..)"

	int nCol1Inx = StackEntry.Find(',');
	if (nCol1Inx > -1)
	{
		int nCol2Inx = StackEntry.Find(',', nCol1Inx+1);
		if (nCol2Inx > -1)
		{
			StackEntry.SetAt(nCol1Inx, '(');
			StackEntry.SetAt(nCol2Inx, ')');
			StackEntry.Insert(nCol2Inx+1, ' ');
		}
		
		int nEndPathInx = nCol1Inx;
		while ((nEndPathInx > 0) && (StackEntry[nEndPathInx] != '\\'))
			nEndPathInx--;
		
		if (nEndPathInx > 0)
			StackEntry = StackEntry.Right(StackEntry.GetLength() - nEndPathInx - 1);
	}
}


void CStackViewBar::OnDblclickStackList()
{
	int nSel = m_List.GetCurSel();
	if (nSel > -1)
	{
		int i = 0; 
		const char* pCur = &m_vecCallStack[0];
		while ( i < nSel )
		{
			int Len = strlen(pCur);
			if (Len == 0)
				break;
			pCur += Len + 1;
			i++;
		}
		
		if (i == nSel)
		{
			CString PathFile = pCur;
			int nPathEnd = PathFile.Find(',');
			if (nPathEnd > 0)
			{
				PathFile.SetAt(nPathEnd, '\0');
				int nLineNum = strtol(pCur + nPathEnd + 1, NULL, 10);
				CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
				pApp->DisplayFileLine(PathFile, nLineNum);
			}
		}
	}
}

