//
// ModeScenario
//

#include "stdafx.h"
#include "globals.h"

#include "Mode.h"
#include "Startup.h"
#include "SysMap.h"
#include "Scenario.h"
#include "Campaign.h"
#include "Crc32Static.h"
#include "SystemStructs.h"
#include "resource.h"
#include "Editor.h"
#include "EditorDoc.h"
#include "EditorView.h"
#include "SystemProps.h"
#include "SplitWnd.h"
#include "HKEvent.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <EventSys.h>
#include <system.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

class ModeScenario : public IMode, public IEventCallback
{
public:

	BEGIN_DACOM_MAP_INBOUND(ModeScenario)
		DACOM_INTERFACE_ENTRY(IMode)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	ModeScenario()
	{
		eventHandle = 0;
		bInit = false;
	}

	~ModeScenario()
	{
		if( bInit )
		{
			// do uninit
		}

		eventHandle = 0;
		bInit = false;
	}

	virtual bool OnCreate( LPCREATESTRUCT lpcs, CCreateContext* pContext ){ return true; }
	virtual bool Start();
	virtual bool Stop();
	virtual void Update();
	virtual void Draw();

	// IEventCallback methods

	DEFMETHOD(Notify) (U32 message, void *param);

	// locals

	enum
	{
		WINDOW_ID = WM_USER + 102,
	};

	U32			  eventHandle;
	bool		  bInit;
	CTreeCtrl     m_SystemTree;
	SystemProps   m_SystemProps;
	CWnd          m_childWindow;
	CxSplitterWnd m_SplitWnd;

	void resetSystemTree();
	void editSystemInfo( DWORD _systemID );
	void editScript( DWORD _scriptID );
	void updateSystem( DWORD _systemID );
	void setWindowVisible( bool _bSetting );

	// message handlers
	void onButtonUp( MSG* );
	void onButtonDblClk( MSG* );
	void onNotify( MSG* );

	HTREEITEM findParentItem( HTREEITEM _item )
	{
		HTREEITEM item = m_SystemTree.GetParentItem(_item);
		if( !item )
		{
			return _item;
		}
		return findParentItem( item );
	}
};

//-----------------------------------------------------------------------------------------------------

bool ModeScenario::Start()
{ 
	CQEDITORMODE = EM_SCENARIO;

	if( !bInit )
	{
		bInit = true;

		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
		if( frame )
		{
			CView* view = frame->GetActiveView();
			if( view )
			{
				DWORD dwFlags = CS_VREDRAW | CS_HREDRAW;
				CString strMyClass = AfxRegisterWndClass( dwFlags, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)::GetStockObject(NULL_BRUSH), ::LoadIcon(NULL, IDI_APPLICATION) );

				DWORD dwStyle = WS_CHILD;
				CRect rect(0,0,0,0);
				m_childWindow.Create( strMyClass, "scenario_window", dwStyle, rect, view, WINDOW_ID );
			}

			if( m_SplitWnd.CreateStatic( &m_childWindow, 2, 1 ) )
			{
				// make sure to NOT make these "View" classes, in order to decouple from MFC Frame/View framework
				CRuntimeClass* runtimeClass = RUNTIME_CLASS(CWnd);

				CCreateContext context;
				context.m_pCurrentFrame	  = frame;
				context.m_pCurrentDoc	  = frame->GetActiveDocument();
				context.m_pNewDocTemplate = context.m_pCurrentDoc->GetDocTemplate();;
				context.m_pNewViewClass	  = runtimeClass;
				context.m_pLastView       = frame->GetActiveView();

				if( m_SplitWnd.CreateView(0,0,runtimeClass,CSize(0,0),&context) )
				{
					DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
					m_SystemTree.Create( dwStyle, CRect(0,0,0,0), m_SplitWnd.GetPane(0,0), 0 );
				}

				if( m_SplitWnd.CreateView(1,0,runtimeClass,CSize(0,0),&context) )
				{
					int t = 5;
				}
			}

			frame->SetActiveView( view );
		}
	}

	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		setWindowVisible( true );

		CView* thisView = frame->GetActiveView();
		if( thisView )
		{
			::ShowScrollBar( thisView->m_hWnd, SB_BOTH, false );

			CRect rect;
			thisView->GetClientRect( rect );
			m_childWindow.SetWindowPos( NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0 );
		}

		CRect rect;
		m_childWindow.GetClientRect( rect );
		m_SplitWnd.SetWindowPos( NULL, rect.left, rect.top, rect.Width(), rect.Height(), 0 );

		m_SplitWnd.GetClientRect( rect );
		m_SplitWnd.SetRowInfo( 0, rect.Height()/2, rect.Height()/4 );
		m_SplitWnd.SetRowInfo( 1, rect.Height()/2, rect.Height()/4 );
		m_SplitWnd.RecalcLayout();

		CWnd* pane0 = m_SplitWnd.GetPane(0,0);
		if( pane0 )
		{
			// set up tree info
			CRect paneRect;
			pane0->GetClientRect( paneRect );
			m_SystemTree.SetWindowPos( NULL, paneRect.left, paneRect.top, paneRect.Width(), paneRect.Height(), 0 );
		}

		frame->SetActiveView( thisView );
		frame->InvalidateRect(NULL);
	}

	resetSystemTree();
	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool ModeScenario::Stop()
{ 
	setWindowVisible( false );
	return true;
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::Update()
{
	if( !bInit )
	{
		Start();
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::Draw()
{
}

//-----------------------------------------------------------------------------------------------------

GENRESULT ModeScenario::Notify(U32 message, void *param)
{
	if( CQEDITORMODE == EM_SCENARIO )
	{
		if( message < WM_USER )
		{
			MSG* pMsg = (MSG*)param;

			if( pMsg->hwnd == m_SystemTree.m_hWnd )
			{
				if( pMsg->message == WM_LBUTTONUP)
				{
					onButtonUp( pMsg );
				}
				else if( pMsg->message == WM_LBUTTONDBLCLK )
				{
					onButtonDblClk( pMsg );
				}
				else if( pMsg->message == WM_NOTIFY )
				{
					onNotify( pMsg );
				}
			}

			else if( pMsg->hwnd == m_SystemProps.m_hWnd )
			{
				if( pMsg->message == WM_DESTROY )
				{
					if( pMsg->lParam == IDOK )
					{
						updateSystem( pMsg->wParam );
					}

					m_SystemProps.EndDialog(pMsg->lParam);
					m_SystemProps.DestroyWindow();
				}
			}

		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::onButtonUp( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	CPoint pt(xPos,yPos);
	HTREEITEM item = m_SystemTree.HitTest(pt);
	if( item )
	{
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::onButtonDblClk( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	CPoint pt(xPos,yPos);
	HTREEITEM item = m_SystemTree.HitTest(pt);
	if( item )
	{
		HTREEITEM parent = findParentItem( item );

		// chose a valid child?
		if( parent && item != parent )
		{
			CString szSystems; szSystems.LoadString(IDS_SYSTEMS);
			CString szScripts; szScripts.LoadString(IDS_SCRIPTS);

			if( m_SystemTree.GetItemText(parent) == szSystems )
			{
				editSystemInfo( m_SystemTree.GetItemData(item) );
			}
			else if( m_SystemTree.GetItemText(parent) == szScripts )
			{
				editScript( m_SystemTree.GetItemData(item) );
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::onNotify( MSG* _pMsg )
{
	int idCtrl = (int) _pMsg->wParam; 
	LPNMHDR pnmh = (LPNMHDR) _pMsg->lParam;

//	if( pnmh->code == TVN_ENDLABELEDIT )
//	{
//		LPNMTVDISPINFO info = (LPNMTVDISPINFO)pnmh;
//
//		CString szName(info->item.pszText);
//
//		// update item
//		HTREEITEM item = m_ScenarioTree.GetSelectedItem();
//		if( item )
//		{
//			m_ScenarioTree.SetItemText( item, szName );
//		}
//
//		IScenario* pScenario = (IScenario*)info->item.lParam;
//		if( pScenario )
//		{
//			// upate sceneraio name?
//
//			// todo(aaj-4/26/2004): true UNICODE code
//			::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, szName, szName.GetLength()+1, pScenario->GetSettings().name, 120);
//		}
//		else if( item == m_ScenarioTree.GetRootItem() )
//		{
//			// update campaign name
//
//			// todo(aaj-4/26/2004): true UNICODE code
//			::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, szName, szName.GetLength()+1, CAMPAIGN->GetSettings().name, 120);
//		}
//	}
//	else if( pnmh->code == TVN_KEYDOWN )
//	{
//		LPNMTVKEYDOWN keydown = (LPNMTVKEYDOWN)pnmh;
//
//		if( keydown->wVKey == VK_DELETE )
//		{
//			HTREEITEM item = m_ScenarioTree.GetSelectedItem();
//			if( item )
//			{
//				IScenario* pScenario = (IScenario*)m_ScenarioTree.GetItemData(item);
//				if( pScenario )
//				{
//					if( query(CString("Delete Scenario?"),CString("Are you sure?")) == IDOK )
//					{
//						CAMPAIGN->RemoveScenario( pScenario );
//					}
//				}
//			}
//		}
//	}
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::resetSystemTree()
{
	// clearing tree to start all over
	m_SystemTree.DeleteAllItems();

	ISector* sector = NULL;
	if( CAMPAIGN && CAMPAIGN->GetCurrentScenario() )
	{
		sector = CAMPAIGN->GetCurrentScenario()->GetActiveSector();
	}

	CString szSystems; szSystems.LoadString(IDS_SYSTEMS);
	CString szScripts; szScripts.LoadString(IDS_SCRIPTS);

	HTREEITEM systemsFolder = m_SystemTree.InsertItem(szSystems, 0, 0);
	HTREEITEM scriptsFolder = m_SystemTree.InsertItem(szScripts, 0, 0);

	if( sector )
	{
		for( int sysIdx = 0; sysIdx < MAX_SYSTEMS+1; sysIdx++ )
		{
			System* system = sector->FindSystemByIdx(sysIdx);
			if( system && !system->bEmpty )
			{
				CString szName(system->name);
				if( szName != _T("") )
				{
					HTREEITEM item = m_SystemTree.InsertItem( TVIF_TEXT | TVIF_PARAM, szName, 0, 0, 0, 0, sysIdx, systemsFolder, TVI_LAST ); 
					if( item )
					{
						m_SystemTree.InsertItem( TVIF_TEXT | TVIF_PARAM, system->backgroundName, 0, 0, 0, 0, sysIdx, item, TVI_LAST ); 
						m_SystemTree.InsertItem( TVIF_TEXT | TVIF_PARAM, system->systemKitName, 0, 0, 0, 0, sysIdx, item, TVI_LAST ); 
					}
				}
			}
		}
	}

	// susposed to have children
	if( m_SystemTree.ItemHasChildren(systemsFolder) == false )
	{
		m_SystemTree.DeleteItem(systemsFolder);
	}

	// susposed to have children
	if( m_SystemTree.ItemHasChildren(scriptsFolder) == false )
	{
		m_SystemTree.DeleteItem(scriptsFolder);
	}
}

//----------------------------------------------------------------------------------------------

void ModeScenario::editSystemInfo( DWORD _systemID )
{
	CWnd* wnd = m_SplitWnd.GetPane(1,0);
	if( wnd )
	{
		// cancel out of last system props, if any
		if( m_SystemProps.m_hWnd )
		{
			m_SystemProps.EndDialog(IDCANCEL);
			m_SystemProps.DestroyWindow();
		}

		// set this system to be edit
		m_SystemProps.SetSystemData( Editor::GetSystem(0,_systemID) );

		// create window for system properties
		if( m_SystemProps.Create( MAKEINTRESOURCE(IDD_SYSTEMPROPS), wnd ) )
		{
			CRect rect;
			wnd->GetClientRect(rect);

			m_SystemProps.SetWindowPos( wnd, rect.top, rect.left, rect.Width(), rect.Height(), 0 );
			m_SystemProps.ShowWindow( SW_NORMAL );
		}
	}
}

//----------------------------------------------------------------------------------------------

void ModeScenario::editScript( DWORD _scriptID )
{
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::updateSystem( DWORD _systemID )
{
	HTREEITEM item = m_SystemTree.GetRootItem();
	while( item )
	{
		CString szSystems; szSystems.LoadString(IDS_SYSTEMS);

		if( m_SystemTree.GetItemText(item) == szSystems )
		{
			HTREEITEM systemItem = m_SystemTree.GetChildItem(item);
			while( systemItem )
			{
				if( m_SystemTree.GetItemData(systemItem) == _systemID )
				{
					// update the children

					HTREEITEM child = m_SystemTree.GetChildItem(systemItem);

					while( child )
					{
						HTREEITEM next = m_SystemTree.GetNextSiblingItem(child);
						m_SystemTree.DeleteItem(child);
						child = next;
					}

					System* system = Editor::GetSystem(0,_systemID);
					if( system )
					{
						m_SystemTree.SetItemText( systemItem, system->name );
						m_SystemTree.InsertItem( TVIF_TEXT | TVIF_PARAM, system->backgroundName, 0, 0, 0, 0, _systemID, systemItem, TVI_LAST ); 
						m_SystemTree.InsertItem( TVIF_TEXT | TVIF_PARAM, system->systemKitName, 0, 0, 0, 0, _systemID, systemItem, TVI_LAST ); 
					}
				}

				systemItem = m_SystemTree.GetNextSiblingItem(systemItem);
			}
		}

		item = m_SystemTree.GetNextSiblingItem(item);
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeScenario::setWindowVisible( bool _bSetting )
{
	if( m_SystemProps.m_hWnd )
	{
		m_SystemProps.EndDialog(IDCANCEL);
		m_SystemProps.DestroyWindow();
	}

	U32 showWindowType = _bSetting ? SW_NORMAL : SW_HIDE;

	m_SystemTree.ShowWindow( showWindowType );

	CWnd* pane1 = m_SplitWnd.GetPane(1,0);
	if( pane1 )
		pane1->ShowWindow( showWindowType );

	CWnd* pane0 = m_SplitWnd.GetPane(0,0);
	if( pane0 )
		pane0->ShowWindow( showWindowType );

	m_SplitWnd.ShowWindow( showWindowType );

	m_childWindow.ShowWindow( showWindowType );
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _ModeScenario : GlobalComponent
{
	ModeScenario * mode;

	virtual void Startup (void)
	{
		MODE_SCENARIO = mode = new DAComponent<ModeScenario>;
		AddToGlobalCleanupList((IDAComponent **) &MODE_SCENARIO);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise( static_cast<IEventCallback *>(mode), &mode->eventHandle);
		}
	}
};
static _ModeScenario __ModeScenario;
