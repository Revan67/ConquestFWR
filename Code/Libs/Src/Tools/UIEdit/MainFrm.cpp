// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "UIEdit.h"

#include "MainFrm.h"
#include "UIArtFileView.h"
#include "UIArtFileListView.h"
#include "UIRectEditView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_CBN_DROPDOWN(IDC_ARTFILE_LISTBOX, OnDropdownArtfileListbox)
	ON_CBN_SELENDOK(IDC_ARTFILE_LISTBOX, OnSelEndOkArtfileListbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	if (!m_wndArtToolBar.Create(this, IDD_DIALOGBAR, CBRS_TOP, IDD_DIALOGBAR))
	{
		TRACE0("Failed to create art tools dialog bar\n");
		return -1;      // fail to create
	}

	// TODO: Remove this if you don't want tool tips or a resizeable toolbar
	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	m_wndArtToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndArtToolBar);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class

#if 0
	if (m_wndSplitter.CreateStatic (this, 2, 1))
	{
		// Create the views
		if (pContext != NULL && pContext->m_pNewViewClass != NULL)
		{
			SIZE s;
			s.cx = s.cy = 100;

			if (m_wndSplitter.CreateView(0, 0, RUNTIME_CLASS(CUIArtFileListView), s, pContext) == NULL)
				return FALSE;

			if (m_wndSplitter.CreateView(1, 0, pContext->m_pNewViewClass, s, pContext) == NULL)
				return FALSE;

			return TRUE;
		}
	}
#endif
	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

void CMainFrame::OnDropdownArtfileListbox()
{
	CComboBox *cb = (CComboBox *) m_wndArtToolBar.GetDlgItem (IDC_ARTFILE_LISTBOX);
	if (cb)
	{
		CUIEditDoc *pDoc = (CUIEditDoc *) GetActiveDocument();
		if (pDoc)
		{
			// Fill the list box with the names of the artfiles in the current document.
			cb->ResetContent();

			int count = pDoc->m_Data.arts.GetCount();
			if (count)
			{
				while(count--)
				{
					UIHandle h;
					ArtFile *afp = pDoc->m_Data.getArtByIndex(count, h);
					if (afp)
					{
						int index = cb->AddString(afp->name);
						if (index != CB_ERR && index != CB_ERRSPACE)
						{
							cb->SetItemData(index, h);
						}
					}
				}
			}
		}
	}
}

void CMainFrame::OnSelEndOkArtfileListbox()
{
	CComboBox *cb = (CComboBox *) m_wndArtToolBar.GetDlgItem (IDC_ARTFILE_LISTBOX);
	if (cb)
	{
		CUIEditDoc *pDoc = (CUIEditDoc *) GetActiveDocument();
		if (pDoc)
		{
			// Get the current selection.
			int index = cb->GetCurSel();
			if (index == CB_ERR)
			{
				pDoc->m_CurrentArt = UIHANDLE_INVALID;
			}
			else
			{
				pDoc->m_CurrentArt = cb->GetItemData(index);
			}

			pDoc->UpdateAllViews(NULL);
		}
	}
}
