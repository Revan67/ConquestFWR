// UIArtFileListView.cpp : implementation file
//

#include "stdafx.h"
#include "uiedit.h"
#include "UIArtFileListView.h"
#include "UIEditDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Constants
//

const int ART_ICON_SIZE = 64;
const int ART_INITIAL_COUNT = 5;
const int ART_GROW_COUNT = 5;

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileListView

IMPLEMENT_DYNCREATE(CUIArtFileListView, CFormView)

CUIArtFileListView::CUIArtFileListView()
	: CFormView(CUIArtFileListView::IDD)
{
	//{{AFX_DATA_INIT(CUIArtFileListView)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CUIArtFileListView::~CUIArtFileListView()
{
}

void CUIArtFileListView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUIArtFileListView)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUIArtFileListView, CFormView)
	//{{AFX_MSG_MAP(CUIArtFileListView)
	ON_COMMAND(ID_TEST_ART_FILE, OnTestArtFile)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ARTFILE_LIST, OnItemchangedArtfileList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileListView diagnostics

#ifdef _DEBUG
void CUIArtFileListView::AssertValid() const
{
	CFormView::AssertValid();
}

void CUIArtFileListView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CUIEditDoc* CUIArtFileListView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CUIEditDoc)));
	return (CUIEditDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileListView utility methods

void CUIArtFileListView::syncToCurrentArt(CUIEditDoc* pDoc)
{
	// Sync the bitmap to the current item in the dialog
	CStatic *bmImage = (CStatic *) GetDlgItem(IDC_ART_IMAGE);
	if (bmImage)
	{
		ArtFile *afp = NULL;
		if (pDoc->m_CurrentArt != UIHANDLE_INVALID)
		{
			afp = pDoc->m_Data.getArtFile(pDoc->m_CurrentArt);
		}
	
		if (afp)
		{
			bmImage->SetBitmap(afp->bitmap);
		}
		else
		{
			bmImage->SetBitmap(NULL);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// CUIArtFileListView message handlers

void CUIArtFileListView::OnTestArtFile() 
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
			pDoc->m_CurrentArt = pDoc->m_Data.addArtFile (afp);
			pDoc->SetModifiedFlag();

			// Add a new item to the list control for this file.
			CListCtrl *pList = (CListCtrl *) GetDlgItem(IDC_ARTFILE_LIST);
			if (pList)
			{
				pList->InsertItem(0, fd.GetFileName());

				CString buffer;
				pList->SetItemData(0, pDoc->m_CurrentArt);
				buffer.Format("%d", afp->w);
				pList->SetItemText(0, 1, buffer);
				buffer.Format("%d", afp->h);
				pList->SetItemText(0, 2, buffer);
			}

			// Display the currently selected art, i.e the one just entered.
			syncToCurrentArt(pDoc);

			// Update any other views as needed.
			pDoc->UpdateAllViews (this);
		}
	}
}


void CUIArtFileListView::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();
	
	// TODO: Add your specialized code here and/or call the base class
	
	// *** TODO: Move this elsewhere, since this is called on new documents as well as on
	// *** creation.

	// Setup the sub-items for the control
	CListCtrl *pList = (CListCtrl *) GetDlgItem(IDC_ARTFILE_LIST);
	if (pList)
	{
		pList->InsertColumn(0, "Filename", LVCFMT_LEFT, 100, 0);
		pList->InsertColumn(1, "Width", LVCFMT_LEFT, 50, 1);
		pList->InsertColumn(2, "Height", LVCFMT_LEFT, 50, 2);
	}
}


void CUIArtFileListView::OnItemchangedArtfileList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	// If the state has changed, and the item is getting selected, update the current art
	// value for the document and update all the views.

	if (pNMListView->uChanged & LVIF_STATE)
	{
		CUIEditDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);

		if ((pNMListView->uNewState & LVIS_SELECTED) && !(pNMListView->uOldState & LVIS_SELECTED))
		{
			// Set the document's current art to this art, then update all views.

			CListCtrl *pList = (CListCtrl *) GetDlgItem(IDC_ARTFILE_LIST);
			if (pList)
			{
				pDoc->m_CurrentArt = pList->GetItemData(pNMListView->iItem);
				syncToCurrentArt(pDoc);
				pDoc->UpdateAllViews(this);
			}
		}
		else if (!(pNMListView->uNewState & LVIS_SELECTED) && (pNMListView->uOldState & LVIS_SELECTED))
		{
			// NULL out (unselect) the current art.
			pDoc->m_CurrentArt = UIHANDLE_INVALID;
			syncToCurrentArt(pDoc);
			pDoc->UpdateAllViews(this);
		}
	}
	
	*pResult = 0;
}

void CUIArtFileListView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	// TODO: Add your specialized code here and/or call the base class

}
