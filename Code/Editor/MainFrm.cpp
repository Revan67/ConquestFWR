// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "globals.h"

#include "Editor.h"
#include "MainFrm.h"
#include <EventSys.h>
#include "Mode.h"
#include "Scenario.h"
#include "Campaign.h"
#include "tinyxml\tinyxml.h"
#include "SaveLoad.h"
#include "StringEditor.h"
#include "StringTable.h"

#include <TSmartPointer.H>
#include <FileSys.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

namespace Editor
{
	IMode* getMode()
	{
		switch( CQEDITORMODE )
		{
			case EM_CAMPAIGN:	return MODE_CAMPAIGN;
			case EM_SCENARIO:	return MODE_SCENARIO;
			case EM_SECTOR:		return MODE_SECTOR;
			case EM_SYSTEM:		return MODE_SYSTEM;
		}
		return MODE_SYSTEM;
	}
}

//----------------------------------------------------------------------------------------------

enum IMAGE
{
	CampaignImage,
	ScenarioImage,
	SectorImage,
	SystemImage,
	
	IMAGE_COUNT
};

static HICON s_Images[IMAGE_COUNT];

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_MODE_CAMPAIGN, OnModeCampaign)
	ON_COMMAND(ID_MODE_SCENARIO, OnModeScenario)
	ON_COMMAND(ID_MODE_SECTOR, OnModeSector)
	ON_COMMAND(ID_MODE_START, OnModeStart)
	ON_COMMAND(ID_MODE_SYSTEM, OnModeSystem)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_NEW_SCENARIO, OnNewScenario)
	ON_COMMAND(ID_SCENARIO_OPEN, OnScenarioOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_SAVESENARIO, OnFileSavesenario)
	ON_COMMAND(ID_FILE_SAVEASMISSION, OnFileSaveasmission)
	ON_COMMAND(ID_PLAYER_PLAYERONE, OnPlayerPlayerone)
	ON_COMMAND(ID_PLAYER_PLAYERTWO, OnPlayerPlayertwo)
	ON_COMMAND(ID_PLAYER_PLAYERTHREE, OnPlayerPlayerthree)
	ON_COMMAND(ID_PLAYER_PLAYERFOUR, OnPlayerPlayerfour)
	ON_COMMAND(ID_PLAYER_PLAYERFIVE, OnPlayerPlayerfive)
	ON_COMMAND(ID_PLAYER_PLAYERSIX, OnPlayerPlayersix)
	ON_COMMAND(ID_PLAYER_PLAYERSEVEN, OnPlayerPlayerseven)
	ON_COMMAND(ID_PLAYER_PLAYEREIGHT, OnPlayerPlayereight)
	ON_COMMAND(ID_STRING_TABLE, OnStringTable)
	ON_COMMAND(ID_OBJECTCOMMAND_DELETE, OnObjectcommandDelete)
	ON_COMMAND(ID_OBJECTCOMMAND_RENAME, OnObjectcommandRename)
	ON_COMMAND(ID_OBJECTCOMMAND_CLONE, OnObjectcommandClone)
	ON_COMMAND(ID_OBJECTCOMMAND_PROPERTIES, OnObjectcommandProperties)
	ON_COMMAND(ID_OBJECTCOMMAND_WARP, OnObjectcommandWarp)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_OBJECTCOMMAND_GOTO, OnObjectcommandGoto)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_MODE_CAMPAIGN_VIEW, OnModeCampaignView)
	ON_COMMAND(ID_MODE_SCENARIO_VIEW, OnModeScenarioView)
	ON_COMMAND(ID_MODE_SECTOR_VIEW, OnModeSectorView)
	ON_COMMAND(ID_MODE_SYSTEM_VIEW, OnModeSystemView)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_FILE_EXPORTSTRINGTABLE, OnFileExportstringtable)
	ON_COMMAND(ID_FILE_IMPORTSTRINGTABLE, OnFileImportstringtable)
	ON_COMMAND(ID_GROUPCOMMAND_ADDTO, OnGroupcommandAddto)
	ON_COMMAND(ID_OBJECTCOMMAND_SELECT, OnObjectcommandSelect)
	ON_COMMAND(ID_MAP_LAUNCH, OnMapLaunch)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	IDS_PLAYER_NUM,
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_currentPlayerID = 0;
}
/////////////////////////////////////////////////////////////////////////////
// 
CMainFrame::~CMainFrame()
{
}
/////////////////////////////////////////////////////////////////////////////
// 
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	EnableDocking(CBRS_ALIGN_ANY);
	
	// mode image list

	s_Images[CampaignImage] = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\campaign_mode.ico", 0 );
	s_Images[ScenarioImage] = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\scenario_mode.ico", 0 );
	s_Images[SectorImage]   = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\sector_mode.ico", 0 );
	s_Images[SystemImage]   = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\system_mode.ico", 0 );

	if( !m_modeImageList.Create( 32, 32, ILC_MASK | ILC_COLOR32, 0, 0) )
	{
		// error
		return -1;
	}
	m_modeImageList.SetImageCount( IMAGE_COUNT );
	m_modeImageList.Replace( CampaignImage, s_Images[CampaignImage] );
	m_modeImageList.Replace( ScenarioImage, s_Images[ScenarioImage] );
	m_modeImageList.Replace( SectorImage,   s_Images[SectorImage] );
	m_modeImageList.Replace( SystemImage,   s_Images[SystemImage] );

	// editor mode bar

	// flat buttons grouped together
	DWORD dwFlags = WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC;
	DWORD dwStyle = TBSTYLE_FLAT | BTNS_CHECK | BTNS_GROUP;

	if( !m_editorToolBar.CreateEx(this, dwStyle, dwFlags) )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;
	}
	m_editorToolBar.GetToolBarCtrl().SetImageList( &m_modeImageList );
	m_editorToolBar.EnableDocking( CBRS_ALIGN_ANY );
	m_editorToolBar.SetButtons( NULL, 4 );
	m_editorToolBar.SetButtonInfo( CampaignImage, ID_MODE_CAMPAIGN, dwStyle, EM_CAMPAIGN );
	m_editorToolBar.SetButtonInfo( ScenarioImage, ID_MODE_SCENARIO, dwStyle, EM_SCENARIO );
	m_editorToolBar.SetButtonInfo( SectorImage,   ID_MODE_SECTOR,   dwStyle, EM_SECTOR );
	m_editorToolBar.SetButtonInfo( SystemImage,   ID_MODE_SYSTEM,   dwStyle, EM_SYSTEM );

	CRect temp;
	m_editorToolBar.GetItemRect(0,&temp);
	m_editorToolBar.SetSizes(CSize(temp.Width(),temp.Height()),CSize(32,32));

	m_editorToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_editorToolBar);

	// status bar

	if( !m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)) )
	{
		TRACE0("Failed to create status bar\n");
		return -1;
	}
	UpdateStatusBar();

	// all other mode bars

	int sideBarWidth = 1000;

	// left side
//	SetupBar( m_sectorBar, IDS_SECTORBAR, CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_LEFT, CRect(0,0,300,300), NULL);
//	SetupBar( m_systemBar, IDS_SYSTEMBAR, CBRS_ALIGN_ANY, AFX_IDW_DOCKBAR_LEFT, CRect(0,0,300,300), NULL );
	SetupBar( m_assetBar,  IDS_ASSETBAR,  CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT, AFX_IDW_DOCKBAR_LEFT, CRect(0,0,300,300), NULL);

//	DockControlBarNextTo(&m_sectorBar, &m_systemBar);
//	DockControlBarNextTo(&m_systemBar, &m_assetBar);

	// right side
	SetupBar( m_entityBar, IDS_ENTITYBAR, CBRS_ALIGN_LEFT |	CBRS_ALIGN_RIGHT, AFX_IDW_DOCKBAR_RIGHT, CRect(0,0,300,300), NULL);
	SetupBar( m_groupBar,  IDS_GROUPS,    CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT, AFX_IDW_DOCKBAR_RIGHT, CRect(0,0,300,300), NULL);
	DockControlBarNextTo(&m_entityBar, &m_groupBar);

	return 0;
}
/////////////////////////////////////////////////////////////////////////////
// 
void CMainFrame::SetupBar( CSidebar& _bar, U32 _idsString, U32 _cbrsAlignFlags, U32 _afxIdwDockbarAlign, CRect& _rect, CSidebar* _nextTo )
{
	if( !_bar.name.LoadString( _idsString ) || !_bar.Create(_bar.name, this, _idsString) )
	{
		CString failMsg = _bar.name + CString(" - FAILED to make\n");
		TRACE0( failMsg );
		return;
	}

	_bar.SetBarStyle( _bar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	_bar.EnableDocking( _cbrsAlignFlags );

	DockControlBar( &_bar, _afxIdwDockbarAlign, _rect );

	m_sidebars.Add( &_bar );
}
/////////////////////////////////////////////////////////////////////////////
// 
void CMainFrame::DockControlBarNextTo(CControlBar* pBar, CControlBar* pTargetBar)
{
	ASSERT(pBar != NULL);
	ASSERT(pTargetBar != NULL);
	ASSERT(pBar != pTargetBar);

	// the neighbour must be already docked
	CDockBar* pDockBar = pTargetBar->m_pDockBar;
	ASSERT(pDockBar != NULL);
	UINT nDockBarID = pTargetBar->m_pDockBar->GetDlgCtrlID();
	ASSERT(nDockBarID != AFX_IDW_DOCKBAR_FLOAT);

	bool bHorz = (nDockBarID == AFX_IDW_DOCKBAR_TOP || nDockBarID == AFX_IDW_DOCKBAR_BOTTOM);

	// dock normally (inserts a new row)
	DockControlBar(pBar, nDockBarID);

	// delete the new row (the bar pointer and the row end mark)
	pDockBar->m_arrBars.RemoveAt(pDockBar->m_arrBars.GetSize() - 1);
	pDockBar->m_arrBars.RemoveAt(pDockBar->m_arrBars.GetSize() - 1);

	// find the target bar
	for (int i = 0; i < pDockBar->m_arrBars.GetSize(); i++)
	{
		void* p = pDockBar->m_arrBars[i];

		// and insert the new bar after it
		if (p == pTargetBar) 
		{
			pDockBar->m_arrBars.InsertAt(i + 1, pBar);
		}
	}

	// add "end row mark"
	pDockBar->m_arrBars.InsertAt(i + 1, (void*)NULL);

	// move the new bar into position
	CRect rBar;
	pTargetBar->GetWindowRect(rBar);
	rBar.OffsetRect(bHorz ? 1 : 0, bHorz ? 0 : 1);
	pBar->MoveWindow(rBar);
}
/////////////////////////////////////////////////////////////////////////////
// 
void CMainFrame::UpdateStatusBar()
{
	if( m_currentPlayerID != Editor::playerID )
	{
		m_currentPlayerID = Editor::playerID;

		CString format;
		format.LoadString(IDS_PLAYER_NUM);
		CString playerNum;
		playerNum.Format( format, m_currentPlayerID );
		m_wndStatusBar.SetPaneText( 0, playerNum, true );

		CMenu* menu = GetMenu();
		menu->CheckMenuItem( ID_PLAYER_PLAYERONE,	Editor::playerID == 1 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERTWO,	Editor::playerID == 2 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERTHREE, Editor::playerID == 3 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERFOUR,	Editor::playerID == 4 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERFIVE,	Editor::playerID == 5 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERSIX,	Editor::playerID == 6 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYERSEVEN, Editor::playerID == 7 ? MF_CHECKED : MF_UNCHECKED );
		menu->CheckMenuItem( ID_PLAYER_PLAYEREIGHT, Editor::playerID == 8 ? MF_CHECKED : MF_UNCHECKED );
	}

	CMenu* menu = GetMenu();
	menu->CheckMenuItem( ID_MODE_CAMPAIGN_VIEW,	CQEDITORMODE == EM_CAMPAIGN ? MF_CHECKED : MF_UNCHECKED );
	menu->CheckMenuItem( ID_MODE_SCENARIO_VIEW,	CQEDITORMODE == EM_SCENARIO ? MF_CHECKED : MF_UNCHECKED );
	menu->CheckMenuItem( ID_MODE_SECTOR_VIEW,	CQEDITORMODE == EM_SECTOR   ? MF_CHECKED : MF_UNCHECKED );
	menu->CheckMenuItem( ID_MODE_SYSTEM_VIEW,	CQEDITORMODE == EM_SYSTEM   ? MF_CHECKED : MF_UNCHECKED );

	m_editorToolBar.SendMessage( TB_SETSTATE, ID_MODE_CAMPAIGN, CQEDITORMODE == EM_CAMPAIGN ? TBSTATE_PRESSED : 0 );
	m_editorToolBar.SendMessage( TB_SETSTATE, ID_MODE_SCENARIO, CQEDITORMODE == EM_SCENARIO ? TBSTATE_PRESSED : 0 );
	m_editorToolBar.SendMessage( TB_SETSTATE, ID_MODE_SECTOR,   CQEDITORMODE == EM_SECTOR   ? TBSTATE_PRESSED : 0 );
	m_editorToolBar.SendMessage( TB_SETSTATE, ID_MODE_SYSTEM,   CQEDITORMODE == EM_SYSTEM   ? TBSTATE_PRESSED : 0 );
}
/////////////////////////////////////////////////////////////////////////////
// 
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
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

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) 
{
	if( EVENTSYS )
	{
		EVENTSYS->Send( pMsg->message, pMsg );
	}
	return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	for( int i = 0; i < m_sidebars.GetSize(); i++ )
	{
		m_sidebars[i]->DoPaint(dc);
	}

	// Do not call CFrameWnd::OnPaint() for painting messages
}

void CMainFrame::OnDestroy() 
{
	CFrameWnd::OnDestroy();
}

BOOL CMainFrame::DestroyWindow() 
{
	CSizingControlBar::GlobalSaveState( this, "CQ2_CONTROL_BARS");
	return CFrameWnd::DestroyWindow();
}

void CMainFrame::ResetBars()
{
	for( int i = 0; i < m_sidebars.GetSize(); i++ )
	{
		m_sidebars[i]->Reset();
	}
}

void CMainFrame::UpdateBars()
{
	for( int i = 0; i < m_sidebars.GetSize(); i++ )
	{
		m_sidebars[i]->Update();
	}
}

void CMainFrame::OnEditUndo() 
{
	if( EVENTSYS )
		EVENTSYS->Send(CQE_UNDO);
}

void CMainFrame::OnEditRedo() 
{
	if( EVENTSYS )
		EVENTSYS->Send(CQE_REDO);
}

void CMainFrame::OnEditCopy()
{
	if( EVENTSYS )
		EVENTSYS->Send(CQE_COPY);
}

void CMainFrame::OnEditPaste()
{
	if( EVENTSYS )
		EVENTSYS->Send(CQE_PASTE);
}

void CMainFrame::OnEditCut()
{
	if( EVENTSYS )
		EVENTSYS->Send(CQE_CUT);
}

void CMainFrame::OnModeCampaign() 
{
	IMode * mode = Editor::getMode();
	if( mode )
	{
		mode->Stop();
	}
	MODE_CAMPAIGN->Start();
	UpdateStatusBar();
}

void CMainFrame::OnModeScenario() 
{
	IMode * mode = Editor::getMode();
	if( mode )
	{
		mode->Stop();
	}
	MODE_SCENARIO->Start();
	UpdateStatusBar();
}

void CMainFrame::OnModeSector() 
{
	IMode * mode = Editor::getMode();
	if( mode )
	{
		mode->Stop();
	}
	MODE_SECTOR->Start();
	UpdateStatusBar();
}

void CMainFrame::OnModeStart() 
{
}

void CMainFrame::OnModeSystem() 
{
	IMode * mode = Editor::getMode();
	if( mode )
	{
		mode->Stop();
	}
	MODE_SYSTEM->Start();
	UpdateStatusBar();
}

void CMainFrame::OnFileNew() 
{
	if( EVENTSYS )
	{
		EVENTSYS->Send(CQE_NEW_CAMPAIGN);
	}
}

void CMainFrame::OnFileOpen()
{
	// loading a campaign
	CArray<CString,CString> fileList;

	if( Editor::GetOpenFileList(fileList) )
	{
		if( fileList.GetSize() > 0 )
		{
			CString filename = fileList.GetAt(0);
			
			TiXmlDocument doc;
			if( doc.LoadFile(filename) )
			{
				COMPTR<ISaverLoader> loader;
				if( CAMPAIGN->QueryInterface("ISaverLoader", loader) == GR_OK )
				{
					if( loader->Load(doc) )
					{
						extern CEditorApp theApp;
						theApp.AddToRecentFileList(filename);
					}
				}
			}
		}
	}
}

void CMainFrame::OnNewScenario() 
{
	IScenario* s = Scenario::New();
	if( s )
	{
		s->NewSector(L"NEW_SECTOR");
		CAMPAIGN->AddScenario( s );
	}
}

void CMainFrame::OnFileSavesenario()
{
	IScenario* s = CAMPAIGN->GetCurrentScenario();

	if( s )
	{
		CString filename( s->GetSettings().name );
		filename += ".scenario";
		s->Save(filename);
	}
}

void CMainFrame::OnScenarioOpen() 
{
	CArray<CString,CString> fileList;

	if( Editor::GetOpenFileList(fileList) )
	{
		for( int i = 0; i < fileList.GetSize(); i++ )
		{
			CString filename = fileList.GetAt(i);

			IScenario* s = Scenario::New();
			if( s->Load(filename) )
			{
				CAMPAIGN->AddScenario(s);

				// assign at least one scenario as active
				if( CAMPAIGN->GetCurrentScenario() == NULL )
				{
					CAMPAIGN->SetCurrentScenario( s );
				}

				extern CEditorApp theApp;
				theApp.AddToRecentFileList(filename);
			}
			else
			{
				delete s;
			}
		}
	}
}

void CMainFrame::OnFileSave() 
{
	int t = 5;
}

void CMainFrame::OnFileSaveasmission()
{
	CString path = "Z:\\CQ2\\Code\\App\\Maps\\";
	// TODO : figure a way to compute this from the App's info

	IScenario* s = CAMPAIGN->GetCurrentScenario();

	if( s )
	{
		CString fn( s->GetSettings().name );
		fn += ".qmission";

		CString filename(path);
		filename += fn;

		COMPTR<IFileSystem> f;
		DAFILEDESC fdesc = filename;
		fdesc.lpImplementation = "UTF";
		fdesc.dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		fdesc.dwShareMode = 0;  // no sharing
		fdesc.dwCreationDistribution = CREATE_ALWAYS;
		if( DACOM->CreateInstance(&fdesc,f) == GR_OK )
		{
			IFileSystem* fs = f;
			s->Save( *fs );
			f->CloseHandle(0);
		}
	}
}

void CMainFrame::OnPlayerPlayerone()
{
	Editor::playerID = 1;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayertwo()
{
	Editor::playerID = 2;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayerthree()
{
	Editor::playerID = 3;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayerfour()
{
	Editor::playerID = 4;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayerfive()
{
	Editor::playerID = 5;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayersix()
{
	Editor::playerID = 6;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayerseven()
{
	Editor::playerID = 7;
	UpdateStatusBar();
}

void CMainFrame::OnPlayerPlayereight()
{
	Editor::playerID = 8;
	UpdateStatusBar();
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
	CFrameWnd::ActivateFrame(nCmdShow);
}

void CMainFrame::OnMenu()
{
}

void CMainFrame::OnStringTable()
{
	StringEditor e;
	e.DoModal();
}

//----------------------------------------------------------------------------------------------
// The Context Menu (SystemMode)

void CMainFrame::OnObjectcommandDelete()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_DELETE );
}

void CMainFrame::OnObjectcommandRename()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_RENAME );
}

void CMainFrame::OnObjectcommandClone()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_CLONE );
}

void CMainFrame::OnObjectcommandProperties()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_PROPERTIES );
}

void CMainFrame::OnObjectcommandWarp()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_WARP );
}

void CMainFrame::OnObjectcommandGoto()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_GOTO );
}

void CMainFrame::OnGroupcommandAddto()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_ADDTOGROUP );
}

void CMainFrame::OnObjectcommandSelect()
{
	if( EVENTSYS )
		EVENTSYS->Send( CEQ_CONTEXT_EVENT, (void*)CQE_CE_SELECT );
}

//----------------------------------------------------------------------------------------------
// Menu | Modes

void CMainFrame::OnModeCampaignView()
{
	OnModeCampaign();
	UpdateStatusBar();
}

void CMainFrame::OnModeScenarioView()
{
	OnModeScenario();
	UpdateStatusBar();
}

void CMainFrame::OnModeSectorView()
{
	OnModeSector();
	UpdateStatusBar();
}

void CMainFrame::OnModeSystemView()
{
	OnModeSystem();
	UpdateStatusBar();
}

//----------------------------------------------------------------------------------------------
// File | Import/Export string table

void CMainFrame::OnFileExportstringtable()
{
	CString filename;
	if( Editor::GetSaveFile(filename) )
	{
		CString szExtension = ".stringTable";
		if( filename.Right(szExtension.GetLength()) != szExtension )
		{
			filename += szExtension;
		}

		TiXmlDocument doc( filename );
		doc.InsertEndChild( TiXmlDeclaration("1.0","","yes") );

		COMPTR<ISaverLoader> stringTableSaver;
		STRINGTABLE->QueryInterface( "ISaverLoader", stringTableSaver );
		if( stringTableSaver )
		{
			stringTableSaver->Save(doc);
		}

		doc.SaveFile();
	}
}

void CMainFrame::OnFileImportstringtable()
{
	CArray<CString,CString> fileList;
	if( Editor::GetOpenFileList(fileList) )
	{
		if( fileList.GetSize() > 0 )
		{
			CString filename = fileList.GetAt(0);

			CString szExtension = ".stringTable";
			if( filename.Right(szExtension.GetLength()) != szExtension )
			{
				return;
			}
			
			TiXmlDocument doc;
			if( doc.LoadFile(filename) )
			{
				COMPTR<ISaverLoader> loader;
				if( STRINGTABLE->QueryInterface("ISaverLoader", loader) == GR_OK )
				{
					loader->Load(doc);
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	if( MODE_CAMPAIGN ) MODE_CAMPAIGN->OnCreate(lpcs,pContext);
	if( MODE_SCENARIO ) MODE_SCENARIO->OnCreate(lpcs,pContext);
	if( MODE_SECTOR   ) MODE_SECTOR->OnCreate(lpcs,pContext);
	if( MODE_SYSTEM   ) MODE_SYSTEM->OnCreate(lpcs,pContext);

	return CFrameWnd::OnCreateClient(lpcs, pContext);
}

void CMainFrame::OnContextMenu(CWnd* pWnd, CPoint point)
{
}

BOOL CMainFrame::OnCommand(WPARAM wParam, LPARAM lParam)
{
	return CFrameWnd::OnCommand(wParam, lParam);
}

void CMainFrame::OnMapLaunch()
{
	// TODO : figure a way to compute this from the App's info

	CString pathMap = "Z:\\CQ2\\Code\\App\\Maps\\";
	CString pathApp = "Z:\\CQ2\\Code\\App\\Src";

	IScenario* s = CAMPAIGN->GetCurrentScenario();

	if( s )
	{
		CString fn( s->GetSettings().name );
		fn += ".qmission";

		CString mapname(pathMap);
		mapname += fn;

		CString app(pathApp);
		app += "\\Debug\\Conquest.exe";

		if( ::GetFileAttributes(mapname) != 0XFFFFFFFF && ::GetFileAttributes(app) != 0XFFFFFFFF )
		{
			CString szCmdLine = app + CString(" ") + mapname;
			LPSTR lpCmdLine = szCmdLine.GetBuffer(0);

			STARTUPINFO         StartInfo;
			PROCESS_INFORMATION ProcessInfo;
			BOOL                result;
			   
			memset(&StartInfo,   0, sizeof(StartInfo));
			memset(&ProcessInfo, 0, sizeof(ProcessInfo));

			StartInfo.cb = sizeof(StartInfo);

			result = CreateProcess( NULL,         // Image name
									lpCmdLine,    // Command line
									NULL,         // Process security
									NULL,         // Thread security
									FALSE,        // Do not inherit handles
									0,            // Creation flags
									NULL,         // Inherit parent environment
									pathApp,      // Keep current working directory
									&StartInfo,   // Startup info structure
									&ProcessInfo);// Process info structure

			result++;
		}
	}
}
