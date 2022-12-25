// UIEditDoc.cpp : implementation of the CUIEditDoc class
//

#include "stdafx.h"
#include "UIEdit.h"

#include "UIEditDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CUIEditDoc

IMPLEMENT_DYNCREATE(CUIEditDoc, CDocument)

BEGIN_MESSAGE_MAP(CUIEditDoc, CDocument)
	//{{AFX_MSG_MAP(CUIEditDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIEditDoc construction/destruction

CUIEditDoc::CUIEditDoc()
{
	// TODO: add one-time construction code here

}

CUIEditDoc::~CUIEditDoc()
{
}

BOOL CUIEditDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	// Set the default canvas size.
	size.cx = 640;
	size.cy = 480;

	// Set the current art and rect handles to invalid.
	m_CurrentArt = UIHANDLE_INVALID;
	m_CurrentRect = UIHANDLE_INVALID;
	m_EditMode = EDIT_RECTS;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CUIEditDoc serialization

void CUIEditDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here

	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CUIEditDoc diagnostics

#ifdef _DEBUG
void CUIEditDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CUIEditDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIEditDoc commands
