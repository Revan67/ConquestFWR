// EntityBar

#include "stdafx.h"
#include "globals.h"

#include "EntityBar.h"

#include "Scenario.h"
#include "SystemStructs.h"
#include "CQTrace.h"
#include "GameTypes.h"
#include "Campaign.h"
#include "System.h"
#include "Object.h"
#include "EventSys.h"
#include "resource.h"
#include "camera.h"
#include "Editor.h"

#include <TComponent.h>
#include <TSmartPointer.h>
#include <Startup.h>
#include <EventSys.h>
#include <system.h>
#include <IConnection.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static CEntityBar* THIS_CEntityBar = NULL;

/////////////////////////////////////////////////////////////////////////////
// CSidebar

BEGIN_MESSAGE_MAP(CEntityBar, CSidebar)
	//{{AFX_MSG_MAP(CEntityBar)
	ON_WM_CREATE()
	ON_WM_PARENTNOTIFY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// 

enum IMAGE
{
	FolderOpen,
	FolderClosed,
	Player1,
	Player2,
	Player3,
	Player4,
	Player5,
	Player6,
	Player7,
	Player8,

	IMAGE_COUNT
};

static HICON s_Images[IMAGE_COUNT];

/////////////////////////////////////////////////////////////////////////////
// message handlers

int CEntityBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	THIS_CEntityBar = this;
	m_bNeedUpdate = false;

	s_Images[FolderOpen]	= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\folder_open.ico", 0 );
	s_Images[FolderClosed]	= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\folder_close.ico", 0 );
	s_Images[Player1]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_yellow.ico", 0 );
	s_Images[Player2]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_red.ico", 0 );
	s_Images[Player3]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_blue.ico", 0 );
	s_Images[Player4]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_pink.ico", 0 );
	s_Images[Player5]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_green.ico", 0 );
	s_Images[Player6]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_orange.ico", 0 );
	s_Images[Player7]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_purple.ico", 0 );
	s_Images[Player8]		= ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\player_cyan.ico", 0 );

	if (CSidebar::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// create image list
	if( !m_imageList.Create( 16, 16, ILC_MASK | ILC_COLOR32, 0, 0) )
	{
		// error
		return -1;
	}
	m_imageList.SetImageCount( IMAGE_COUNT );
	m_imageList.Replace( FolderOpen,   s_Images[FolderOpen] );
	m_imageList.Replace( FolderClosed, s_Images[FolderClosed] );
	m_imageList.Replace( Player1,	   s_Images[Player1] );
	m_imageList.Replace( Player2,	   s_Images[Player2] );
	m_imageList.Replace( Player3,	   s_Images[Player3] );
	m_imageList.Replace( Player4,	   s_Images[Player4] );
	m_imageList.Replace( Player5,	   s_Images[Player5] );
	m_imageList.Replace( Player6,	   s_Images[Player6] );
	m_imageList.Replace( Player7,	   s_Images[Player7] );
	m_imageList.Replace( Player8,	   s_Images[Player8] );

	DWORD id = 'ESTB';
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;

	if( !m_treeView.Create( dwStyle, CRect(0,0,0,0), this, id ) )
	{
		return -1;
	}
	m_treeView.SetImageList( &m_imageList, TVSIL_NORMAL );
	m_treeView.SetMultiSelect(true);

	m_currentSystem = NULL;

	return 0;
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::OnParentNotify(UINT message, LPARAM lParam) 
{
	CSidebar::OnParentNotify(message, lParam);
	
	if( message == WM_RBUTTONDOWN )
	{
		CPoint pt(lParam);
		HTREEITEM item = m_treeView.HitTest(pt);
		if( !item )
		{
			return;
		}

		// make sure this is an "object"
		if( m_treeView.ItemHasChildren(item) )
		{
			return;
		}

		// first select this object
		OBJECTSELECTION->Reset();
		OBJECTSELECTION->Add( (IObject*)m_treeView.GetItemData(item) );

		// then move the context menu to this window's item
		CRect rect;
		::GetWindowRect( m_treeView.m_hWnd, rect );
		pt.x += rect.left;
		pt.y += rect.top;

		// start context menu
		if( EVENTSYS )
		{
			EVENTSYS->Send( CEQ_START_CONTEXT, (void*)&pt );
		}
	}

}

//-----------------------------------------------------------------------------------------------------

BOOL CEntityBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* hdr = (NMHDR*)lParam;

	if( hdr->hwndFrom == m_treeView.m_hWnd )
	{
		if( hdr->code == TVN_SELCHANGED )
		{
			m_bNeedUpdate = true;
		}
	}

	return CSidebar::OnNotify(wParam, lParam, pResult);
}

//-----------------------------------------------------------------------------------------------------

CEntityBar::~CEntityBar()
{
	m_currentSystem = NULL;
}

//-----------------------------------------------------------------------------------------------------

bool CEntityBar::Update()
{
	if( CAMPAIGN->GetCurrentScenario() && CAMPAIGN->GetCurrentScenario()->GetActiveSector() )
	{
		System* system = CAMPAIGN->GetCurrentScenario()->GetActiveSector()->GetActiveSystem();

		if( system )
		{
			if( system != m_currentSystem )
			{
				m_currentSystem = system;
				m_treeView.DeleteAllItems();
			}

			if( m_currentSystem )
			{
				// add all new objects form sytem object list
				HTREEITEM itemRoot = m_treeView.GetRootItem();
				for( ObjectList::iterator it = system->objectList.begin(); it != system->objectList.end(); it++ )
				{
					insertObject( itemRoot, *it );
				}

				// add all wormhole objects
				for( int jumpIndex = 0; jumpIndex < system->jList.GetCount(); jumpIndex++ )
				{
					JumpPoint& point = system->jList.ElementAt(jumpIndex);
					if( point.wormholeObject )
					{
						insertObject( itemRoot, point.wormholeObject );
					}
				}

				// remove all old objects
				HTREEITEM item = m_treeView.GetRootItem();
				while( item )
				{
					removeObject( item );
					item = m_treeView.GetNextItem(item,TVGN_NEXT);
				}

				// prune branches that have no children anymore
				HTREEITEM branch = m_treeView.GetRootItem();
				while( branch )
				{
					HTREEITEM next = m_treeView.GetNextSiblingItem(branch);

					if( m_treeView.ItemHasChildren(branch) == false )
					{
						m_treeView.DeleteItem(branch);
					}

					branch = next;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

int CEntityBar::DoPaint( CPaintDC& _dc )
{
	CPaintDC dc(this); // device context for painting
	return -1;
}

//----------------------------------------------------------------------------------------------

HTREEITEM CEntityBar::findFolder( IObject* obj, U32& _iconImage )
{
	ObjectData data;
	obj->GetObjectData(data);

	// unknown icon
	_iconImage = FolderClosed;

	if( data.objectClass == OC_SPACESHIP || data.objectClass == OC_PLATFORM )
	{
		// assign the icon type based on player ID
		// the only two types of objects that have player ID are ships and platforms
		_iconImage = data.playerID + 1;
	}

	HTREEITEM item = m_treeView.GetRootItem();

	while( item )
	{
		if( m_treeView.ItemHasChildren(item) )
		{
			OBJCLASS objclass = (OBJCLASS)m_treeView.GetItemData(item);

			if( objclass & data.objectClass )
			{
				return item;
			}
		}
		item = m_treeView.GetNextItem(item,TVGN_NEXT);
	}

	// need a new folder
	switch( data.objectClass )
	{
		case OC_SPACESHIP:
		case OC_PLATFORM:
			item = m_treeView.InsertItem("Objects", 0, 0);
			m_treeView.SetItemData( item, OC_SPACESHIP | OC_PLATFORM );
			break;

		case OC_BLACKHOLE:
			item = m_treeView.InsertItem("Black Holes", 0, 0);
			m_treeView.SetItemData( item, OC_BLACKHOLE );
			break;

		case OC_PLANETOID:
			item = m_treeView.InsertItem("Planets", 0, 0);
			m_treeView.SetItemData( item, OC_PLANETOID );
			break;

		case OC_JUMPGATE:
			item = m_treeView.InsertItem("Wormholes", 0, 0);
			m_treeView.SetItemData( item, OC_JUMPGATE );
			break;

		case OC_MINEFIELD:
		case OC_NEBULA:
		case OC_FIELD:
			item = m_treeView.InsertItem("Fields", 0, 0);
			m_treeView.SetItemData( item, OC_MINEFIELD | OC_NEBULA | OC_FIELD );
			break;

		case OC_WAYPOINT:
		case OC_PLAYERBOMB:
		case OC_TRIGGER:
		case OC_SCRIPTOBJECT:
			item = m_treeView.InsertItem("Triggers", 0, 0);
			m_treeView.SetItemData( item, OC_WAYPOINT | OC_PLAYERBOMB | OC_TRIGGER | OC_SCRIPTOBJECT );
			break;

		case OC_NONE:
		case OC_MEXPLODE:
		case OC_SHRAPNEL:
		case OC_LAUNCHER:
		case OC_WEAPON:
		case OC_BLAST:
		case OC_FIGHTER:
		case OC_LIGHT:
		case OC_TRAIL:
		case OC_EFFECT:
		case OC_NUGGET:
		case OC_GROUP:
		case OC_RESEARCH:
		case OC_BUILDRING:
		case OC_BUILDOBJ:
		case OC_MOVIECAMERA:
		case OC_UI_ANIM:
			item = m_treeView.GetRootItem();
			break;
	}

	return item;
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::insertObject( HTREEITEM _item, IObject* _obj )
{
	// find the correct folder for this object
	U32 imageIdx = 0;
	HTREEITEM folder = findFolder(_obj, imageIdx);

	ObjectData data;
	_obj->GetObjectData( data );

	if( folder )
	{
		HTREEITEM item = m_treeView.GetChildItem(folder);
		while( item )
		{
			if( m_treeView.ItemHasChildren(item) == false )
			{
				IObject* pObject = (IObject*)m_treeView.GetItemData(item);

				if( pObject == _obj )
				{
					// no need to insert, but update name
					CString oldName = m_treeView.GetItemText(item);

					if( oldName != data.scriptHandle )
					{
						m_treeView.SetItemText( item, data.scriptHandle );
					}

					return;
				}
			}
			item = m_treeView.GetNextItem(item,TVGN_NEXT);
		}
	}

	DWORD mask = TVIF_PARAM | TVIF_TEXT | TVIF_HANDLE;
	HTREEITEM newItem = m_treeView.InsertItem( mask, data.scriptHandle, 0, 0, 0, 0, (LPARAM)_obj, folder, TVI_LAST );
	m_treeView.SetItemImage( newItem, imageIdx, imageIdx );
	CQASSERT( newItem );
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::removeObject( HTREEITEM _item )
{
	if( m_treeView.ItemHasChildren(_item) )
	{
		HTREEITEM hNextItem;
		HTREEITEM hChildItem = m_treeView.GetChildItem(_item);

		while (hChildItem != NULL)
		{
			hNextItem = m_treeView.GetNextItem(hChildItem, TVGN_NEXT);
			removeObject( hChildItem );
			hChildItem = hNextItem;
		}
	}
	else
	{
		IObject* obj = (IObject*)m_treeView.GetItemData(_item);

		for( ObjectList::iterator it = m_currentSystem->objectList.begin(); it != m_currentSystem->objectList.end(); it++ )
		{
			if( obj == *it )
			{
				return;
			}
		}

		// add all wormhole objects
		for( int jumpIndex = 0; jumpIndex < m_currentSystem->jList.GetCount(); jumpIndex++ )
		{
			JumpPoint& point = m_currentSystem->jList.ElementAt(jumpIndex);
			if( point.wormholeObject == obj )
			{
				return;
			}
		}

		m_treeView.DeleteItem( _item );
	}
}

//-----------------------------------------------------------------------------------------------------

bool CEntityBar::selectObject( HTREEITEM _item, IObject* _object )
{
	if( _item )
	{
		if( m_treeView.ItemHasChildren(_item) )
		{
			HTREEITEM hNextItem;
			HTREEITEM hChildItem = m_treeView.GetChildItem(_item);

			while (hChildItem != NULL)
			{
				hNextItem = m_treeView.GetNextItem(hChildItem, TVGN_NEXT);
				
				if( selectObject(hChildItem,_object) )
				{
					return true;
				}
				
				hChildItem = hNextItem;
			}
		}
		else
		{
			IObject* obj = (IObject*)m_treeView.GetItemData(_item);

			if( obj == _object )
			{
				m_treeView.SetItemState(_item, TVIS_SELECTED, TVIS_SELECTED);
				m_treeView.EnsureVisible(_item);
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

GENRESULT CEntityBar::Notify(U32 message, void *param)
{
	// selecting an object (from EntityBar for example)
	if( message == CQE_ENTITY_SELECT )
	{
		// deselect all items
		m_treeView.SelectAll(false);

		// select all objects in object selection
		if( OBJECTSELECTION->HasObjects() )
		{
			ObjectQuickList list;
			OBJECTSELECTION->GetList(list);

			for( ObjectQuickList::iterator it = list.begin(); it != list.end(); it++ )
			{
				selectObject( m_treeView.GetRootItem(), *it );
			}
		}
	}
	else if( message == CQE_UPDATE )
	{
		if( m_bNeedUpdate )
		{
			m_bNeedUpdate = false;

			// update selection based on tree's selection
			updateSelectedList( m_treeView.GetRootItem() );
		}
	}

	return GR_OK;
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::onNotify( MSG* pMsg )
{
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::addImage( const char* _imagename, U32 _rgb )
{
	CBitmap bitmap;
	if( Editor::LoadBMPImage(_imagename, bitmap, NULL ) )
	{
		m_imageList.Add( &bitmap, _rgb );
	}
}

//-----------------------------------------------------------------------------------------------------

void CEntityBar::updateSelectedList( HTREEITEM _item )
{
	if( !_item )
		return;

	if( m_treeView.ItemHasChildren(_item) )
	{
		HTREEITEM hNextItem;
		HTREEITEM hChildItem = m_treeView.GetChildItem(_item);

		while (hChildItem != NULL)
		{
			hNextItem = m_treeView.GetNextItem(hChildItem, TVGN_NEXT);
			updateSelectedList( hChildItem );
			hChildItem = hNextItem;
		}

		updateSelectedList( m_treeView.GetNextSiblingItem(_item) );
	}
	else
	{
		IObject* obj = (IObject*)m_treeView.GetItemData(_item);

		if( m_treeView.IsSelected(_item) )
		{
			OBJECTSELECTION->Add( obj );
		}
		else
		{
			OBJECTSELECTION->Remove( obj );
		}
	}
}

//-----------------------------------------------------------------------------------------------------
// notification handler

struct CEntityBar_Notifier : IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(CEntityBar_Notifier)
		DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()

	U32 m_eventHandle;

	DEFMETHOD(Notify) (U32 message, void *param = 0)
	{
		if( CQEDITORMODE == EM_SYSTEM )
		{
			if( message < WM_USER )
			{
				MSG* pMsg = (MSG*)param;
				if( pMsg->message == WM_NOTIFY )
				{
					THIS_CEntityBar->onNotify( pMsg );
				}
			}
		}

		if( THIS_CEntityBar )
		{
			return THIS_CEntityBar->Notify( message, param );
		}

		return GR_OK;
	}
};

//-----------------------------------------------------------------------------------------------------
// startup

struct _CEntityBar_Notifier : GlobalComponent
{
	CEntityBar_Notifier * bar;

	virtual void Startup (void)
	{
		bar = new DAComponent<CEntityBar_Notifier>;
		AddToGlobalCleanupList((IDAComponent **) &bar);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise( static_cast<IEventCallback *>(bar), &bar->m_eventHandle);
		}
	}
};
static _CEntityBar_Notifier __CEntityBar_Notifier;

