//
// ModeCampaign
//

#include "stdafx.h"
#include "globals.h"

#include "Mode.h"
#include "Startup.h"
#include "Campaign.h"
#include "Resource.h"
#include "NewCampaignDlg.h"
#include "Scenario.h"
#include "StringTable.h"
#include "StringEditor.h"
#include "ClipBoard.h"

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

class ModeCampaign : public IMode, public IEventCallback
{
public:

	BEGIN_DACOM_MAP_INBOUND(ModeCampaign)
		DACOM_INTERFACE_ENTRY(IMode)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	ModeCampaign()
	{
		m_campaignSeed = 0;
		eventHandle = 0;
		bInit = false;
		ZeroMemory( &m_lastModified, sizeof(m_lastModified) );
	}

	~ModeCampaign()
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

	// local data

	U32       eventHandle;
	bool      bInit;
	CTreeCtrl m_ScenarioTree;
	FILETIME  m_lastModified;
	DWORD     m_campaignSeed;

	// local methods

	void getLastWriteTime(FILETIME _ftime, CString& _result )
	{
		FILETIME localTime;
		SYSTEMTIME systemTime;
		
		// Convert the last-write time to local time.
		if (!FileTimeToLocalFileTime(&_ftime, &localTime))
		{
			return;
		}
		
		// Convert the local file time from UTC to system time.
		FileTimeToSystemTime(&localTime, &systemTime);
		
		// Build a string showing the date and time.
		char lpszString[128];
		wsprintf(lpszString, "Last Saved: %02d/%02d/%d  %02d:%02d",
			systemTime.wDay, 
			systemTime.wMonth, 
			systemTime.wYear,
			systemTime.wHour, 
			systemTime.wMinute);

		_result = lpszString;
	}

	DWORD getCampaignSeed()
	{
		DWORD seed = 0;

		if( CAMPAIGN->GetNumScenarios() )
		{
			seed |= CAMPAIGN->GetNumScenarios();

			IScenario** sList = (IScenario**)_alloca( CAMPAIGN->GetNumScenarios() * sizeof(IScenario*) );
			if( CAMPAIGN->GetScenarioList( sList, CAMPAIGN->GetNumScenarios() ) )
			{
				for( unsigned i = 0; i < CAMPAIGN->GetNumScenarios(); i++ )
				{
					seed |= (DWORD)sList[i];
				}
			}
		}

		return seed;
	}

	DWORD query( CString& _title, CString& _msg )
	{
		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
		if( frame )
		{
			return frame->MessageBox( _msg, _title, MB_OKCANCEL );
		}
		return IDCANCEL;
	}

	void addScenarioFile( HTREEITEM, IScenario*);
	void onButtonUp( MSG* );
	void onButtonDblClk( MSG* );
	void onNotify( MSG* );
	void onCopy( void );
	void onPaste( void );
	void renameIfDuplicateName( IScenario* );
};

//-----------------------------------------------------------------------------------------------------

bool ModeCampaign::Start()
{ 
	CQEDITORMODE = EM_CAMPAIGN;

	if( !bInit )
	{
		bInit = true;

		CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
		if( frame )
		{
			DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT;
			m_ScenarioTree.Create( dwStyle, CRect(0,0,0,0), frame->GetActiveView(), 0 );
		}
	}

	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
	if( frame )
	{
		CView* view = frame->GetActiveView();
		if( view )
		{
			::ShowScrollBar( view->m_hWnd, SB_BOTH, false );
		}

		CString szName( CAMPAIGN->GetSettings().name );
		frame->GetActiveDocument()->SetPathName( szName );
		frame->GetActiveDocument()->SetTitle( szName );

		CRect rect;
		frame->GetClientRect( rect );
		m_ScenarioTree.SetWindowPos( NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW );
		m_ScenarioTree.ShowWindow(SW_NORMAL);
		frame->InvalidateRect(NULL);
	}

	return false; 
}

//-----------------------------------------------------------------------------------------------------

bool ModeCampaign::Stop()
{ 
	return m_ScenarioTree.ShowWindow(SW_HIDE) != false;
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::Update()
{
	if( !bInit )
	{
		Start();
	}

	bool bNeedUpdate = false;

	// has this campaign been modified?
	ICampaign::Settings& s = CAMPAIGN->GetSettings();
	if( CompareFileTime( &s.lastModified, &m_lastModified) != 0 )
	{
		bNeedUpdate = true;
	}
	else
	{
		bNeedUpdate = (getCampaignSeed() != m_campaignSeed);
	}

	if( bNeedUpdate )
	{
		m_campaignSeed = getCampaignSeed();
		m_lastModified.dwHighDateTime = s.lastModified.dwHighDateTime;
		m_lastModified.dwLowDateTime  = s.lastModified.dwLowDateTime;

		m_ScenarioTree.DeleteAllItems();

		CString szName(s.name);
		HTREEITEM item = m_ScenarioTree.InsertItem(szName, 0, 0);

		CString szSettings;
		szSettings.LoadString(IDS_CSETTINGS);
		HTREEITEM hSettings = m_ScenarioTree.InsertItem( szSettings, 0, 0, item );

		CString szTime;
		getLastWriteTime( s.lastModified, szTime );
		m_ScenarioTree.InsertItem( szTime, 0, 0, hSettings);

		m_ScenarioTree.Expand( m_ScenarioTree.GetRootItem(), TVE_EXPAND );

		if( CAMPAIGN->GetNumScenarios() )
		{
			IScenario** sList = (IScenario**)_alloca( CAMPAIGN->GetNumScenarios() * sizeof(IScenario*) );
			if( CAMPAIGN->GetScenarioList( sList, CAMPAIGN->GetNumScenarios() ) )
			{
				for( unsigned i = 0; i < CAMPAIGN->GetNumScenarios(); i++ )
				{
					addScenarioFile( item, sList[i] );
				}
			}
		}
	}

//	HTREEITEM item = m_ScenarioTree.GetChildItem( m_ScenarioTree.GetRootItem() );
//	while( item )
//	{
//		IScenario* s = (IScenario*)m_ScenarioTree.GetItemData(item);
//		if( s && s == CAMPAIGN->GetCurrentScenario() )
//		{
//			m_ScenarioTree.SelectItem(item);
//		}
//		item = m_ScenarioTree.GetNextSiblingItem(item);
//	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::Draw()
{
}

//-----------------------------------------------------------------------------------------------------

GENRESULT ModeCampaign::Notify(U32 message, void *param)
{
	if( message == CQE_NEW_CAMPAIGN )
	{
		CNewCampaignDlg dlg;
		dlg.m_EditName = L"New Campaign Name";
		if( dlg.DoModal() == IDOK )
		{
			ICampaign::Settings settings;
			::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, dlg.m_EditName, dlg.m_EditName.GetLength()+1, settings.name, 127);
			CAMPAIGN->New( settings.name, &settings );
		}
	}

	if( CQEDITORMODE == EM_CAMPAIGN )
	{
		if( message < WM_USER )
		{
			MSG* pMsg = (MSG*)param;

			if( pMsg->hwnd == m_ScenarioTree.m_hWnd )
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
		}
		else
		{
			if( message == CQE_COPY )
			{
				onCopy();
			}
			else if( message == CQE_PASTE )
			{
				onPaste();
			}
		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::addScenarioFile( HTREEITEM _root, IScenario* _s )
{
	IScenario::Settings& settings = _s->GetSettings();

	// todo(aaj-4/26/2004): true UNICODE code
	CString szName(settings.name);
	::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, szName, szName.GetLength()+1, settings.name, 127);

	HTREEITEM item = m_ScenarioTree.InsertItem( szName, 0, 0, _root );
	if( item )
	{
		m_ScenarioTree.SetItemData( item, (DWORD)_s );
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::onButtonUp( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	CPoint pt(xPos,yPos);
	HTREEITEM item = m_ScenarioTree.HitTest(pt);
	if( item )
	{
		IScenario* s = (IScenario*)m_ScenarioTree.GetItemData(item);
		if( s )
		{
			CAMPAIGN->SetCurrentScenario(s);
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::onButtonDblClk( MSG* _pMsg )
{
	DWORD fwKeys = _pMsg->wParam;        // key flags 
	DWORD xPos = LOWORD(_pMsg->lParam);  // horizontal position of cursor 
	DWORD yPos = HIWORD(_pMsg->lParam);  // vertical position of cursor 

	CPoint pt(xPos,yPos);
	HTREEITEM item = m_ScenarioTree.HitTest(pt);
	if( item )
	{
		IScenario* s = (IScenario*)m_ScenarioTree.GetItemData(item);
		if( s )
		{
			CEdit* edit = m_ScenarioTree.EditLabel(item);
			if( edit )
			{
				CString szName(s->GetSettings().name);
				edit->SetWindowText(szName);
			}
		}
		else if( item == m_ScenarioTree.GetRootItem() )
		{
			CEdit* edit = m_ScenarioTree.EditLabel(item);
			if( edit )
			{
				CString szName( CAMPAIGN->GetSettings().name );
				edit->SetWindowText(szName);
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::onNotify( MSG* _pMsg )
{
	int idCtrl = (int) _pMsg->wParam; 
	LPNMHDR pnmh = (LPNMHDR) _pMsg->lParam;

	if( pnmh->code == TVN_ENDLABELEDIT )
	{
		LPNMTVDISPINFO info = (LPNMTVDISPINFO)pnmh;

		CString szName(info->item.pszText);

		if( szName.IsEmpty() )
		{
			return;
		}

		// update item
		HTREEITEM item = m_ScenarioTree.GetSelectedItem();
		if( item )
		{
			m_ScenarioTree.SetItemText( item, szName );
		}

		IScenario* pScenario = (IScenario*)info->item.lParam;
		if( pScenario )
		{
			// upate sceneraio name?

			// todo(aaj-4/26/2004): true UNICODE code
			::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, szName, szName.GetLength()+1, pScenario->GetSettings().name, 120);
		}
		else if( item == m_ScenarioTree.GetRootItem() )
		{
			// update campaign name
			CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();
			if( frame )
			{
				frame->GetActiveDocument()->SetPathName( szName );
				frame->GetActiveDocument()->SetTitle( szName );
			}

			// todo(aaj-4/26/2004): true UNICODE code
			::MultiByteToWideChar( CP_ACP, MB_USEGLYPHCHARS, szName, szName.GetLength()+1, CAMPAIGN->GetSettings().name, 120);
		}
	}
	else if( pnmh->code == TVN_KEYDOWN )
	{
		LPNMTVKEYDOWN keydown = (LPNMTVKEYDOWN)pnmh;

		if( keydown->wVKey == VK_DELETE )
		{
			HTREEITEM item = m_ScenarioTree.GetSelectedItem();
			if( item )
			{
				IScenario* pScenario = (IScenario*)m_ScenarioTree.GetItemData(item);
				if( pScenario )
				{
					if( query(CString("Delete Scenario?"),CString("Are you sure?")) == IDOK )
					{
						CAMPAIGN->RemoveScenario( pScenario );
					}
				}
			}
		}
		else if( keydown->wVKey == VK_F2 )
		{
			HTREEITEM item = m_ScenarioTree.GetSelectedItem();
			if( item )
			{
				if( item == m_ScenarioTree.GetRootItem() )
				{
					StringEditor stringChooser;

					if( CAMPAIGN->GetSettings().nameTag[0] )
					{
						DWORD stringID = STRINGTABLE->GetStringIdByTag( CAMPAIGN->GetSettings().nameTag );
						stringChooser.SetSelectedString( stringID );
					}

					if( stringChooser.DoModal() == IDOK )
					{
						if( stringChooser.GetSelectedString() != StringEditor::INVALID_STRING )
						{
							const char* tag = STRINGTABLE->GetStringTag( stringChooser.GetSelectedString() );

							if( tag )
							{
								ICampaign::Settings newSettings;
								memcpy( &newSettings, &CAMPAIGN->GetSettings(), sizeof(newSettings) );
								strncpy( newSettings.nameTag, tag, countof(newSettings.nameTag) );
								CAMPAIGN->SetSettings( newSettings );
							}
						}
					}

					CString campname( CAMPAIGN->GetSettings().name );
					m_ScenarioTree.SetItemText( item, campname );
				}
				else if( m_ScenarioTree.GetItemText(item) != "Settings" && m_ScenarioTree.GetParentItem(item) == m_ScenarioTree.GetRootItem() )
				{
					IScenario* selectedScenario = CAMPAIGN->GetCurrentScenario();
					if( selectedScenario )
					{
						StringEditor stringChooser;

						if( selectedScenario->GetSettings().nameTag[0] )
						{
							DWORD stringID = STRINGTABLE->GetStringIdByTag( selectedScenario->GetSettings().nameTag );
							stringChooser.SetSelectedString( stringID );
						}

						if( stringChooser.DoModal() == IDOK )
						{
							if( stringChooser.GetSelectedString() != StringEditor::INVALID_STRING )
							{
								const char* tag = STRINGTABLE->GetStringTag( stringChooser.GetSelectedString() );

								if( tag )
								{
									IScenario::Settings newSettings;
									memcpy( &newSettings, &selectedScenario->GetSettings(), sizeof(newSettings) );
									strncpy( newSettings.nameTag, tag, countof(newSettings.nameTag) );
									selectedScenario->SetSettings( newSettings );
								}
							}
						}

						CString newname( selectedScenario->GetSettings().name );
						m_ScenarioTree.SetItemText( item, newname );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::onCopy( void )
{
	HTREEITEM hItem = m_ScenarioTree.GetSelectedItem();

	if( hItem && m_ScenarioTree.GetItemText(hItem) != "Settings" && m_ScenarioTree.GetParentItem(hItem) == m_ScenarioTree.GetRootItem() )
	{
		IScenario* selectedScenario = (IScenario*)m_ScenarioTree.GetItemData(hItem);
		if( selectedScenario )
		{
			COMPTR<IClipboardObject> clip;
			if( selectedScenario->QueryInterface("IClipboardObject",clip) == GR_OK )
			{
				CLIPBOARD->Copy( *clip.ptr );
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::onPaste( void )
{
	bool didPaste = false;
	IScenario* scenario = Scenario::New();

	COMPTR<IClipboardObject> clip;
	if( scenario->QueryInterface("IClipboardObject",clip) == GR_OK )
	{
		if( CLIPBOARD->HasDataForType(clip->GetType()) )
		{
			if( CLIPBOARD->Paste(*clip.ptr) )
			{
				if( scenario->GetSettings().loaded )
				{
					renameIfDuplicateName(scenario);
					didPaste = CAMPAIGN->AddScenario(scenario);
				}
			}
		}
	}

	if( !didPaste )
	{
		Scenario::Delete( scenario );
	}
}

//-----------------------------------------------------------------------------------------------------

void ModeCampaign::renameIfDuplicateName( IScenario* _scenario )
{
	const wchar_t* newname = _scenario->GetSettings().name;

	// check for duplicate name
	IScenario** sList = (IScenario**)_alloca( CAMPAIGN->GetNumScenarios() * sizeof(IScenario*) );
	if( CAMPAIGN->GetScenarioList( sList, CAMPAIGN->GetNumScenarios() ) )
	{
		for( unsigned i = 0; i < CAMPAIGN->GetNumScenarios(); i++ )
		{					
			if( !wcscmp(newname, sList[i]->GetSettings().name) )
			{
				wcscat( _scenario->GetSettings().name, L"_" );
				renameIfDuplicateName( _scenario );
			}
		}
	}
}

//-----------------------------------------------------------------------------------------------------
// startup

struct _ModeCampaign : GlobalComponent
{
	ModeCampaign * mode;

	virtual void Startup (void)
	{
		MODE_CAMPAIGN = mode = new DAComponent<ModeCampaign>;
		AddToGlobalCleanupList((IDAComponent **) &MODE_CAMPAIGN);
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
static _ModeCampaign __ModeCampaign;
