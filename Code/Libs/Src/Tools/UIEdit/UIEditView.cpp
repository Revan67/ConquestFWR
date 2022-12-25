// UIEditView.cpp : implementation of the CUIEditView class
//

#include "stdafx.h"
#include "UIEdit.h"

#include "UIEditDoc.h"
#include "UIEditView.h"
#include "UIData.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIEditView

IMPLEMENT_DYNCREATE(CUIEditView, CScrollView)

BEGIN_MESSAGE_MAP(CUIEditView, CScrollView)
	//{{AFX_MSG_MAP(CUIEditView)
	ON_WM_ERASEBKGND()
	ON_UPDATE_COMMAND_UI(ID_EDIT_BUTTONS, OnUpdateEditButtons)
	ON_COMMAND(ID_EDIT_BUTTONS, OnEditButtons)
	ON_COMMAND(ID_EDIT_RECTS, OnEditRects)
	ON_UPDATE_COMMAND_UI(ID_EDIT_RECTS, OnUpdateEditRects)
	ON_COMMAND(ID_ART_ADDNEW, OnArtAddnew)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIEditView construction/destruction

CUIEditView::CUIEditView()
{
	// TODO: add construction code here
	UIEditMode  m_EditMode = EDIT_RECTS;
}

CUIEditView::~CUIEditView()
{
}

BOOL CUIEditView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CUIEditView drawing

void CUIEditView::onEditRectDraw   (CDC* pDC)
{
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: add draw code here

	// Draw the art file currently in view.
	if (pDoc->m_CurrentArt)
	{
		ArtFile *afp = pDoc->m_Data.getArtFile(pDoc->m_CurrentArt);

		if (afp)
		{
			UIRect sr;
			sr.x = sr.y = 0;
			sr.w = afp->w;
			sr.h = afp->h;
			afp->drawFrom (pDC->m_hDC, 0, 0, sr);
		}
	}
	else
	{
		CRect r;
		CBrush b(HS_DIAGCROSS, RGB(0,0,0));
		GetClientRect(&r);
		pDC->FillRect(&r, &b);
	}
}

void CUIEditView::onEditScreenDraw (CDC* pDC)
{
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Fill the document with a background pattern.

	CBrush fillBrush (RGB(128,128,128));
	RECT r;
	SIZE s = pDoc->getSize();
	r.left = 0;
	r.top = 0;
	r.right = s.cx;
	r.bottom = s.cy;
	pDC->FillRect (&r, &fillBrush);

	// Draw a line from one end to the other.

	pDC->MoveTo(0,0);
	pDC->LineTo(s.cx, s.cy);
}

void CUIEditView::OnDraw(CDC* pDC)
{
	switch (m_EditMode)
	{
	case EDIT_RECTS:
		onEditRectDraw (pDC);
		break;

	case EDIT_SCREENS:
	default:
		onEditScreenDraw (pDC);
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CUIEditView diagnostics

#ifdef _DEBUG
void CUIEditView::AssertValid() const
{
	CView::AssertValid();
}

void CUIEditView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CUIEditDoc* CUIEditView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUIEditDoc)));
	return (CUIEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIEditView message handlers

void CUIEditView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_EditMode = pDoc->m_EditMode;

	switch (m_EditMode)
	{
	case EDIT_RECTS:
		{
			ArtFile *afp = pDoc->m_Data.getArtFile(pDoc->m_CurrentArt);
			SIZE s;
			if (afp)
			{
				s.cx = afp->w;
				s.cy = afp->h;
			}
			else
			{
				s.cx = 640;
				s.cy = 480;
			}
			SetScrollSizes( MM_TEXT, s );
		}
		break;

	case EDIT_SCREENS:
	default:
		// Set the scroll sizes to match the current document size and the edit mode
		SetScrollSizes( MM_TEXT, pDoc->getSize( ) );
		break;
	}

	// Perform the inherited behavior.
	CScrollView::OnUpdate(pSender, lHint, pHint);
}

BOOL CUIEditView::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	CBrush br( GetSysColor( COLOR_WINDOW ) );
	FillOutsideRect( pDC, &br );
    return TRUE; // Erased
}

void CUIEditView::OnUpdateEditButtons(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable();
	if (GetDocument()->m_EditMode == EDIT_SCREENS)
	{
		pCmdUI->SetCheck(1);
	}
	else
	{
		pCmdUI->SetCheck(0);
	}
}

void CUIEditView::OnUpdateEditRects(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->Enable();
	if (GetDocument()->m_EditMode == EDIT_RECTS)
	{
		pCmdUI->SetCheck(1);
	}
	else
	{
		pCmdUI->SetCheck(0);
	}
}

void CUIEditView::OnEditButtons() 
{
	// TODO: Add your command handler code here
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->m_EditMode = EDIT_SCREENS;
	pDoc->UpdateAllViews(NULL);
}

void CUIEditView::OnEditRects() 
{
	// TODO: Add your command handler code here
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	pDoc->m_EditMode = EDIT_RECTS;
	pDoc->UpdateAllViews(NULL);
}

void CUIEditView::OnArtAddnew() 
{
	// TODO: Add your command handler code here

	// Add a new file to the art list.
	// NOTE: This is ok to do regardless of the current edit mode.
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Ask for the file. If one was selected, add it to the list, make it the current
	// art file, and update the views. Also, automatically go into EDIT_RECTS mode.
	CFileDialog fd(true);
	if (fd.DoModal() == IDOK)
	{
		ArtFile *afp = new ArtFile;
		strcpy (afp->name, fd.GetPathName());
		if (afp->load ())
		{
			pDoc->m_CurrentArt = pDoc->m_Data.addArtFile (afp);
			pDoc->SetModifiedFlag();
			pDoc->m_EditMode = EDIT_RECTS;
			pDoc->UpdateAllViews (NULL);
		}
	}
}
