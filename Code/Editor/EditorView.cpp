// EditorView.cpp : implementation of the CEditorView class
//

#include "stdafx.h"
#include "Editor.h"

#include "EditorDoc.h"
#include "EditorView.h"
#include "globals.h"
#include "MainFrm.h"
#include <IPython.h>

#include <EventSys.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static unsigned TIMER_MAIN_ID = 'MAIN';
static unsigned TIMER_SIZE_ID = 'SIZE';

/////////////////////////////////////////////////////////////////////////////
// CEditorView

IMPLEMENT_DYNCREATE(CEditorView, CView)

BEGIN_MESSAGE_MAP(CEditorView, CView)
	//{{AFX_MSG_MAP(CEditorView)
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditorView construction/destruction

CEditorView::CEditorView()
{
	timerMain = 0;
	timerSize = 0;
}

CEditorView::~CEditorView()
{
}

BOOL CEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_HSCROLL;
	cs.style |= WS_VSCROLL;

	return CView::PreCreateWindow(cs);
}

void CEditorView::OnInitialUpdate() 
{
	CView::OnInitialUpdate();

	Editor::g_Splash.Init( m_hWnd, ::AfxGetApp()->m_hInstance, IDB_SPLASH );
	Editor::g_Splash.Show();

	// see Editor_DACOM.cpp
	bool InitDacom(HWND _hWnd);
	InitDacom( m_hWnd );

	CMainFrame * pFrame = (CMainFrame*)AfxGetApp()->m_pMainWnd;
	pFrame->ResetBars();

//	CFolderTabCtrl& ftc = m_wndFolderTabCtrl;
//	VERIFY(ftc.Create(WS_CHILD|WS_VISIBLE, rc, this, 1));
//	if (m_nIDRes)
//	{
//		ftc.Load(m_nIDRes);
//	}
//	ShowControls(m_cxFolderTabCtrl);

	Editor::g_Splash.Hide();

// to test python component
//	struct MyParams : IPythonEnum
//	{
//		virtual bool QueryParameter( Parameter& _param )
//		{ 
//			if( !strcmp(_param.function,"func2") )
//			{
//				if( _param.parameterIndex == 0 )
//				{
//					_param.type = IPythonEnum::Parameter::LONG;
//					_param.value._long = 11;
//					return true;
//				}
//				else if( _param.parameterIndex == 1 )
//				{
//					_param.type = IPythonEnum::Parameter::LONG;
//					_param.value._long = 12;
//					return true;
//				}
//			}
//			return false; 
//		}
//	};
//	MyParams myParams;
//
//	PYTHON->CreateModule("Test");
//	PYTHON->SetConstant("Test", "one", 1);
//	PYTHON->SetConstant("Test", "two", 2);
//	PYTHON->EnumConstants( myParams, 0 );
//	PYTHON->LoadModule("Z:\\CQ2\\CODE\\MOD.PY");
//	PYTHON->FindFunctionInModule( "mod", "func" );
//	PYTHON->ExecuteFunction("mod", "func", &myParams);
//	PYTHON->ExecuteFunction("mod", "func2", &myParams);

}


/////////////////////////////////////////////////////////////////////////////
// CEditorView drawing

void CEditorView::OnDraw(CDC* pDC)
{
	CEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}

/////////////////////////////////////////////////////////////////////////////
// CEditorView printing

BOOL CEditorView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CEditorView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

void CEditorView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
}

/////////////////////////////////////////////////////////////////////////////
// CEditorView diagnostics

#ifdef _DEBUG
void CEditorView::AssertValid() const
{
	CView::AssertValid();
}

void CEditorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CEditorDoc* CEditorView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEditorDoc)));
	return (CEditorDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CEditorView message handlers

void CEditorView::OnTimer(UINT nIDEvent) 
{
	if( nIDEvent == TIMER_MAIN_ID )
	{
		// see MainLoop.cpp
		void MainLoop();
		MainLoop();
	}
	else if( nIDEvent == TIMER_SIZE_ID )
	{
		if( timerSize )
		{
			KillTimer(TIMER_SIZE_ID);
			timerSize = 0;
		}
		if( EVENTSYS )
		{
			CRect rect;
			GetClientRect( rect );
			POINT pt = { rect.Width(), rect.Height() };
			EVENTSYS->Send( CQE_WINDOW_RESIZE, &pt );
		}
	}
	else
	{
		CView::OnTimer(nIDEvent);
	}
}

void CEditorView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if( timerMain == 0 )
	{
		timerMain = SetTimer(TIMER_MAIN_ID, 10, NULL );
	}
}

BOOL CEditorView::DestroyWindow() 
{
	if( timerMain )
	{
		KillTimer(TIMER_MAIN_ID);
		timerMain = 0;
	}
	if( timerSize )
	{
		KillTimer(TIMER_SIZE_ID);
		timerSize = 0;
	}
	return CView::DestroyWindow();
}

void CEditorView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CQFLAGS.bGameActive = bActivate != false;
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CEditorView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	if( EVENTSYS )
		EVENTSYS->Send( CQE_SET_FOCUS, 0 );
}

void CEditorView::OnKillFocus(CWnd* pNewWnd) 
{
	CView::OnKillFocus(pNewWnd);
	if( EVENTSYS )
		EVENTSYS->Send( CQE_KILL_FOCUS, 0 );
}

void CEditorView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	if( timerSize )
	{
		KillTimer(TIMER_SIZE_ID);
		timerSize = 0;
	}

	// only send out a 3D resize when the size message has not come in for a fraction of a second
	timerSize = SetTimer(TIMER_SIZE_ID, 1000 / 8, NULL);
}

//-----------------------------------------------------------------------------------------------------
// Editor View input code
//-----------------------------------------------------------------------------------------------------

void CEditorView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( EVENTSYS )
	{
		EVENTSYS->Send( CQE_MOUSE_MOVE, &point );
	}
	CView::OnMouseMove(nFlags, point);
}

BOOL CEditorView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult) 
{
	if( EVENTSYS )
	{
		LPNMHDR hdr = (LPNMHDR)lParam;

		MSG msg;
		msg.message = WM_NOTIFY;
		msg.hwnd = hdr->hwndFrom;
		msg.lParam = lParam;
		msg.wParam = wParam;
		msg.pt.x = msg.pt.y = 0;
		msg.time = 0;

		EVENTSYS->Send( msg.message, &msg );
	}

	return CView::OnNotify(wParam, lParam, pResult);
}

void CEditorView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_ALL;

	::GetScrollInfo(m_hWnd, SB_VERT, &si);

	if( nSBCode == SB_BOTTOM )
	{
		si.nPos = si.nMax;
	}
	else if( nSBCode == SB_LINEDOWN )
	{
		si.nPos += (si.nMax / 100);
	}
	else if( nSBCode == SB_LINEUP )
	{
		si.nPos -= (si.nMax / 100);
	}
	else if( nSBCode == SB_PAGEDOWN )
	{
		int offset = si.nMax - si.nPos;
		si.nPos += (offset / 2);
	}
	else if( nSBCode == SB_PAGEUP )
	{
		si.nPos = (si.nPos / 2);
	}
	else if( nSBCode == SB_THUMBTRACK )
	{
		si.nPos = si.nTrackPos;
	}
	else if( nSBCode == SB_TOP )
	{
		si.nPos = si.nMin;
	}
	
	::SetScrollInfo(m_hWnd, SB_VERT, &si, TRUE);

	if( EVENTSYS )
	{
		MSG msg;
		msg.hwnd    = m_hWnd;
		msg.lParam  = 0;
		msg.wParam  = 0;
		msg.message = WM_HSCROLL;

		EVENTSYS->Send( WM_VSCROLL, &msg );
	}

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CEditorView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_ALL;

	::GetScrollInfo(m_hWnd, SB_HORZ, &si);

	if( nSBCode == SB_BOTTOM )
	{
		si.nPos = si.nMax;
	}
	else if( nSBCode == SB_LINEDOWN )
	{
		si.nPos += (si.nMax / 100);
	}
	else if( nSBCode == SB_LINEUP )
	{
		si.nPos -= (si.nMax / 100);
	}
	else if( nSBCode == SB_PAGEDOWN )
	{
		int offset = si.nMax - si.nPos;
		si.nPos += (offset / 2);
	}
	else if( nSBCode == SB_PAGEUP )
	{
		si.nPos = (si.nPos / 2);
	}
	else if( nSBCode == SB_THUMBTRACK )
	{
		si.nPos = si.nTrackPos;
	}
	else if( nSBCode == SB_TOP )
	{
		si.nPos = si.nMin;
	}
	
	::SetScrollInfo(m_hWnd, SB_HORZ, &si, TRUE);

	if( EVENTSYS )
	{
		MSG msg;
		msg.hwnd    = m_hWnd;
		msg.lParam  = 0;
		msg.wParam  = 0;
		msg.message = WM_HSCROLL;

		EVENTSYS->Send( WM_HSCROLL, &msg );
	}

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

//----------------------------------------------------------------------------------------------

void CEditorView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CView::OnLButtonDblClk(nFlags, point);
}

void CEditorView::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CView::OnLButtonDown(nFlags, point);
}

void CEditorView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	CView::OnLButtonUp(nFlags, point);
}

BOOL CEditorView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CEditorView::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	CView::OnRButtonDblClk(nFlags, point);
}

void CEditorView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CView::OnRButtonDown(nFlags, point);
}

void CEditorView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	CView::OnRButtonUp(nFlags, point);
}

BOOL CEditorView::OnCreateAggregates()
{
	return TRUE;
}

CScrollBar* CEditorView::GetScrollBarCtrl(int nBar) const
{
	return CView::GetScrollBarCtrl(nBar);
}
