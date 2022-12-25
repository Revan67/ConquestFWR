// UIArtFileView.cpp : implementation file
//

#include "stdafx.h"
#include "UIEdit.h"
#include "UIArtFileView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileView

IMPLEMENT_DYNCREATE(CUIArtFileView, CScrollView)

CUIArtFileView::CUIArtFileView()
{
	m_CurrentArtHandle = 0;
}

CUIArtFileView::~CUIArtFileView()
{
}


BEGIN_MESSAGE_MAP(CUIArtFileView, CScrollView)
	//{{AFX_MSG_MAP(CUIArtFileView)
	ON_COMMAND(ID_TEST_ART_FILE, OnTestArtFile)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileView drawing

void CUIArtFileView::OnInitialUpdate()
{
	CScrollView::OnInitialUpdate();

	CSize sizeTotal;
	// TODO: calculate the total size of this view
	sizeTotal.cx = sizeTotal.cy = 100;
	SetScrollSizes(MM_TEXT, sizeTotal);
}

void CUIArtFileView::OnDraw(CDC* pDC)
{
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code here

	// Draw the art file currently in view.
	if (m_CurrentArtHandle)
	{
		ArtFile *afp = pDoc->m_Data.getArtFile(m_CurrentArtHandle);

		if (afp)
		{
			UIRect sr;
			sr.x = sr.y = 0;
			sr.w = afp->w;
			sr.h = afp->h;
			afp->drawFrom (pDC->m_hDC, 0, 0, sr);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileView diagnostics

#ifdef _DEBUG
void CUIArtFileView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CUIArtFileView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CUIEditDoc* CUIArtFileView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUIEditDoc)));
	return (CUIEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileView message handlers

void CUIArtFileView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class
	// For now, the current art file is always the first position in the hash.
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	POSITION start = pDoc->m_Data.arts.GetStartPosition();
	if (start)
	{
		ArtFile *afp;
		UIHandle afh;

		pDoc->m_Data.arts.GetNextAssoc (start, afh, afp);
		if (afh && afp)
		{
			// Set the scroll sizes to match the current art file.
			SIZE s;
			s.cx = afp->w;
			s.cy = afp->h;
			SetScrollSizes( MM_TEXT, s );

			// Set the current art handle.
			m_CurrentArtHandle = afh;
		}
		else
		{
			// Set a bogus size.
			CSize sizeTotal;
			sizeTotal.cx = sizeTotal.cy = 100;
			SetScrollSizes(MM_TEXT, sizeTotal);

			// Set a bogus current art handle
			m_CurrentArtHandle = 0;
		}
	}

	// Perform the inherited behavior.
	CScrollView::OnUpdate(pSender, lHint, pHint);
}

void CUIArtFileView::OnTestArtFile() 
{
	// TODO: Add your command handler code here
	
	CUIEditDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// TODO: Add your command handler code here
	CFileDialog fd(true);
	if (fd.DoModal() == IDOK)
	{
		ArtFile *afp = new ArtFile;
		strcpy (afp->name, fd.GetPathName());
		if (afp->load ())
		{
			pDoc->m_Data.addArtFile (afp);
			pDoc->SetModifiedFlag();
			pDoc->UpdateAllViews (NULL);
		}
	}
}

int CUIArtFileView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CScrollView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// TODO: Add your specialized creation code here
	
	return 0;
}
