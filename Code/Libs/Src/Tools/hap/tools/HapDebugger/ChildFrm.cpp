//////////////////////////////////////////////////////////////////////////////
//  File:    ChildFrm.cpp
//
//  Purpose: A custom MDI child frame that handles the display of the current
//  line/col display as well as the read only display (in the status bar)
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

// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdafx.h"
#include "DbgRemote.h"

#include "ChildFrm.h"
#include "DbgRemoteDoc.h"
#include "DbgRemoteView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_READ, OnUpdateReadOnly)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_LINE, OnUpdateLineNum)
ON_WM_NCRBUTTONDBLCLK()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
	// TODO: add member initialization code here
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
	return m_wndSplitter.Create(this,
		2, 1,			// TODO: adjust the number of rows, columns
		CSize(10, 10),	// TODO: adjust the minimum pane size
		pContext, 
		WS_CHILD | WS_VISIBLE |  SPLS_DYNAMIC_SPLIT);
}

/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers
void CChildFrame::OnUpdateReadOnly(CCmdUI *pCmdUI)
{
	CDbgRemoteDoc* pDoc = (CDbgRemoteDoc*)GetActiveDocument();
	if (pDoc)
	{
		pCmdUI->SetText("READ ONLY");
		pCmdUI->Enable(pDoc->ReadOnly());
	}
	else
		pCmdUI->Enable(false);

}


void CChildFrame::OnUpdateLineNum(CCmdUI *pCmdUI)
{	
	CDbgRemoteView* pVw = (CDbgRemoteView*)GetActiveView();
	if (pVw)
	{
		char buff[32] = {0};
		_snprintf(buff, 31, "Ln:%d,Col:%d", pVw->GetCurrentLine(), pVw->GetCurrentCol());
		pCmdUI->SetText(buff);
		pCmdUI->Enable(true);
	}
	else
		pCmdUI->Enable(false);
}


void CChildFrame::OnNcRButtonDblClk(UINT nHitTest, CPoint point)
{
	CDbgRemoteDoc* pDoc = (CDbgRemoteDoc*)GetActiveDocument();
	if (pDoc)
	{
		pDoc->ShowProperties();
	}

	CMDIChildWnd::OnNcRButtonDblClk(nHitTest, point);
}


