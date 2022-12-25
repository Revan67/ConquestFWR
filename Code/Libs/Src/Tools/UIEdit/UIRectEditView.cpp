// UIRectEditView.cpp : implementation file
//

#include "stdafx.h"
#include "uiedit.h"
#include "UIRectEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIRectEditView

IMPLEMENT_DYNCREATE(CUIRectEditView, CScrollView)

CUIRectEditView::CUIRectEditView()
{
}

CUIRectEditView::~CUIRectEditView()
{
}


BEGIN_MESSAGE_MAP(CUIRectEditView, CScrollView)
	//{{AFX_MSG_MAP(CUIRectEditView)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIRectEditView drawing

void CUIRectEditView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize sizeTotal;
	// TODO: calculate the total size of this view
	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);
}

void CUIRectEditView::OnDraw(CDC* pDC)
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

/////////////////////////////////////////////////////////////////////////////
// CUIRectEditView diagnostics

#ifdef _DEBUG
void CUIRectEditView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CUIRectEditView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CUIEditDoc* CUIRectEditView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUIEditDoc)));
	return (CUIEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIRectEditView message handlers

BOOL CUIRectEditView::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	CBrush br( GetSysColor( COLOR_WINDOW ) );
	FillOutsideRect( pDC, &br );
    return TRUE; // Erased
}

void CUIRectEditView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}
