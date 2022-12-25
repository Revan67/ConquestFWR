// GroupBar

#include "stdafx.h"
#include "globals.h"

#include "GroupBar.h"
#include "Editor.h"
#include "NewGroup.h"

#include "Campaign.h"
#include "Object.h"
#include "SystemStructs.h"
#include "Scenario.h"
#include "ObjectFamily.h"

#include <DBaseData.h>

#include <list>
#include <map>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//----------------------------------------------------------------------------------------------
// TODO: implement the popup bar functions like Rename, Delete, and 
//----------------------------------------------------------------------------------------------

struct FamilyGroups : IObjectFamilyEnum
{
	struct Family : std::list<std::string>
	{
		Family::iterator find(std::string _string)
		{
			for( Family::iterator it = begin(); it != end(); it++ )
			{
				if( *it == _string )
				{
					return it;
				}
			}
			return it;
		}
	};

	struct FamilyMap : std::map<std::string,Family>
	{
	};

	FamilyMap familyMap;

	virtual void EnumFamilyInfo( FamilyInfo& _info )
	{
		Family family;
		familyMap[_info.family] = family;
	}

	virtual void EnumObjectInfo( ObjectInfo& _info )
	{
		ObjectData data;
		_info.object->GetObjectData(data);
		const char* scriptHandle = data.scriptHandle;

		FamilyMap::iterator it = familyMap.find( _info.family );

		if( it != familyMap.end() )
		{
			Family& family = it->second;
			family.push_back( scriptHandle );
		}
	}

	HTREEITEM FindFamily( HTREEITEM _item, CTreeCtrl& _tree, const char* _family )
	{
		if( !_item )
		{
			return 0;
		}

		CGroupBar::GroupInfo* info = (CGroupBar::GroupInfo*)_tree.GetItemData(_item);

		if( info->family == _family )
		{
			return _item;
		}
		else if( _tree.ItemHasChildren(_item) )
		{
			HTREEITEM child = _tree.GetChildItem(_item);
			while( child )
			{
				if( FindFamily(child,_tree,_family) )
				{
					return child;
				}
				child = _tree.GetNextSiblingItem(child);
			}
		}

		return FindFamily( _tree.GetNextSiblingItem(_item), _tree, _family );
	}

	HTREEITEM FindObject( HTREEITEM _item, CTreeCtrl& _tree, HTREEITEM _family, const char* _object )
	{
		if( !_item )
		{
			return _item;
		}

		if( _family )
		{
			if( _tree.ItemHasChildren(_family) )
			{
				HTREEITEM child = _tree.GetChildItem(_family);
				while( child )
				{
					if( FindObject(child,_tree,0,_object) )
					{
						return child;
					}
					child = _tree.GetNextSiblingItem(child);
				}
			}
		}
		else
		{
			CGroupBar::GroupInfo* info = (CGroupBar::GroupInfo*)_tree.GetItemData(_item);

			if( info && info->object == _object )
			{
				return _item;
			}
		}

		return 0;
	}
};

static FamilyGroups s_FamilyGroups;

/////////////////////////////////////////////////////////////////////////////
// CGroupBar

BEGIN_MESSAGE_MAP(CGroupBar, CSidebar)
	//{{AFX_MSG_MAP(CGroupBar)
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
	FolderElement,

	IMAGE_COUNT
};

static HICON s_Images[IMAGE_COUNT];

#define STARTUP_MSG "Right Click To Start Group"

//-----------------------------------------------------------------------------------------------------

CGroupBar::~CGroupBar()
{
	deleteData( m_treeView.GetRootItem() );
}

//-----------------------------------------------------------------------------------------------------

int CGroupBar::DoPaint( CPaintDC& )
{
	return -1;
}

//-----------------------------------------------------------------------------------------------------

bool CGroupBar::Update()
{
	if( CAMPAIGN && CAMPAIGN->GetCurrentScenario() )
	{
		s_FamilyGroups.familyMap.clear();
		CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->EnumFamilyList( s_FamilyGroups );
		CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->EnumObjectFamilyInfo( s_FamilyGroups, NULL );

		updateNewGroups( m_treeView.GetRootItem() );
		updateOldGroups( m_treeView.GetRootItem() );

		m_treeView.InvalidateRect(NULL);
		InvalidateRect(NULL);

		return true;
	}
	return false;
}

//----------------------------------------------------------------------------------------------

int CGroupBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CSidebar::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	DWORD id = WM_USER + 103;
	DWORD dwStyle = WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS;

	if( !m_treeView.Create( dwStyle, CRect(0,0,0,0), this, id ) )
	{
		return -1;
	}

	// create image list
	if( !m_imageList.Create( 16, 16, ILC_MASK | ILC_COLOR32, 0, 0) )
	{
		// error
		return -1;
	}
	s_Images[FolderOpen]    = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\group_folder_open.ico", 0 );
	s_Images[FolderClosed]  = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\group_folder_close.ico", 0 );
	s_Images[FolderElement] = ::ExtractIcon( ::AfxGetApp()->m_hInstance, ".\\res\\icons\\folder_element.ico", 0 );

	m_imageList.SetImageCount( IMAGE_COUNT );
	m_imageList.Replace( FolderOpen,    s_Images[FolderOpen] );
	m_imageList.Replace( FolderClosed,  s_Images[FolderClosed] );
	m_imageList.Replace( FolderElement, s_Images[FolderElement] );

	m_treeView.SetImageList( &m_imageList, TVSIL_NORMAL );

	// TODO: localize this!
	m_treeView.InsertItem(STARTUP_MSG);

	return 0;
}

//-----------------------------------------------------------------------------------------------------

void CGroupBar::OnParentNotify(UINT message, LPARAM lParam)
{
	CSidebar::OnParentNotify(message, lParam);
	
	if( message == WM_RBUTTONDOWN )
	{
		CPoint pt(lParam);
		HTREEITEM item = m_treeView.HitTest(pt);

		// then move the context menu to this window's item
		CRect rect;
		::GetWindowRect( m_treeView.m_hWnd, rect );
		pt.x += rect.left;
		pt.y += rect.top;

		if( !item )
		{
			// "add group menu" here
			startNewGroup();
		}
		else
		{
			// start context menu here for group OR object here
			GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(item);
			if( !info )
			{
				// skip
				return;
			}
			else if( !info->family.IsEmpty() )
			{
				showGroupProps(info->family,pt);
			}
			else if( !info->object.IsEmpty() )
			{
				showObjectProps(info->object,pt);
			}
		}
	}
	else if( message == WM_KEYUP )
	{
		int t = 5;
	}
}

//-----------------------------------------------------------------------------------------------------

BOOL CGroupBar::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	NMHDR* hdr = (NMHDR*)lParam;

	if( hdr->hwndFrom == m_treeView.m_hWnd )
	{
		if( hdr->code == TVN_KEYDOWN )
		{
			TV_KEYDOWN* keydown = (TV_KEYDOWN*)hdr;

			if( keydown->wVKey == VK_DELETE )
			{
				HTREEITEM item = m_treeView.GetSelectedItem();

				if( item )
				{
					GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(item);
					if( info )
					{
						if( !info->family.IsEmpty() )
						{
							CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->RemoveFamily(info->family);
						}
						else if( !info->object.IsEmpty() )
						{
							HTREEITEM parentItem = m_treeView.GetParentItem(item);
							if( parentItem )
							{
								GroupInfo* parentInfo = (GroupInfo*)m_treeView.GetItemData(parentItem);

								const char* familyName = parentInfo->family;
								const char* objectName = info->object;

								CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->RemoveObjectFromFamily( familyName, objectName );
							}
						}
						Update();
					}
				}
			}
			else if( keydown->wVKey == VK_SPACE )
			{
				HTREEITEM item = m_treeView.GetSelectedItem();

				if( item )
				{
					GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(item);
					if( info )
					{
						if( !info->family.IsEmpty() )
						{
							CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->SelectGroupForEditor(info->family);

							// todo: could also go to this location like the GOTO thing
						}
					}
				}
			}
		}
	}

	return CSidebar::OnNotify(wParam, lParam, pResult);
}

//-----------------------------------------------------------------------------------------------------

void CGroupBar::startNewGroup()
{
	NewGroup newGroup;
	if( newGroup.DoModal() == IDOK )
	{
		CAMPAIGN->GetCurrentScenario()->GetSettings().objectFamily->AddFamily( newGroup.newGroupName );
		Update();
	}
}

//-----------------------------------------------------------------------------------------------------

void CGroupBar::showGroupProps( CString& _stringGroup, CPoint& _pt )
{
	HMENU hmenu;           // top-level menu 
	HMENU hmenuTrackPopup; // shortcut menu 

	hmenu = LoadMenu(::AfxGetApp()->m_hInstance,MAKEINTRESOURCE(IDR_MENU_GROUP));

	// TrackPopupMenu cannot display the menu bar so get a handle to the first shortcut menu. 
	hmenuTrackPopup = GetSubMenu(hmenu, 0); 

	// make sure to send all WM_COMMAND messages to MainFrame
	CFrameWnd* frame = (CFrameWnd*)::AfxGetApp()->GetMainWnd();

	// Display the shortcut menu. Track the right mouse button.
	DWORD dwFlags = TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON;
	BOOL ret = TrackPopupMenuEx(hmenuTrackPopup, dwFlags, _pt.x, _pt.y, frame->m_hWnd, NULL); 

	// Destroy the menu.
	DestroyMenu(hmenu);
}

//-----------------------------------------------------------------------------------------------------

void CGroupBar::showObjectProps( CString& _stringObject, CPoint& )
{
	IObject* obj = Editor::GetObjectByHandle( _stringObject );

	if( obj )
	{
		// in DialogProc_ObjectProperties.cpp
		extern INT_PTR CALLBACK DialogProc_ObjectProperties( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

		CreateDialogParam( 
			::AfxGetApp()->m_hInstance, 
			MAKEINTRESOURCE(IDD_OBJPROPS),
			::AfxGetApp()->GetMainWnd()->m_hWnd,
			(DLGPROC)DialogProc_ObjectProperties,
			(DWORD)obj );
	}
}

//-----------------------------------------------------------------------------------------------------

bool CGroupBar::updateNewGroups( HTREEITEM _hitem )
{
	if( s_FamilyGroups.familyMap.size() && m_treeView.GetItemText(_hitem) == STARTUP_MSG )
	{
		m_treeView.DeleteAllItems();
	}

	for( FamilyGroups::FamilyMap::iterator fit = s_FamilyGroups.familyMap.begin(); fit != s_FamilyGroups.familyMap.end(); fit++ )
	{
		const char* familyName = fit->first.c_str();

		HTREEITEM familyItem = s_FamilyGroups.FindFamily( m_treeView.GetRootItem(), m_treeView, familyName );

		if( familyItem == 0 )
		{
			GroupInfo* info = new GroupInfo;
			info->family = familyName;

			familyItem = m_treeView.InsertItem( familyName, FolderClosed, FolderOpen );
			m_treeView.SetItemData( familyItem, (DWORD)info );
		}

		FamilyGroups::Family& family = fit->second;

		for( FamilyGroups::Family::iterator child = family.begin(); child != family.end(); child++ )
		{
			const char* scriptHandle = child->c_str();

			HTREEITEM childItem = s_FamilyGroups.FindObject( _hitem, m_treeView, familyItem, scriptHandle );

			if( childItem == 0 )
			{
				GroupInfo* info = new GroupInfo;
				info->object = scriptHandle;
				childItem = m_treeView.InsertItem( scriptHandle, FolderElement, FolderElement, familyItem );
				m_treeView.SetItemData( childItem, (DWORD)info );
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CGroupBar::updateOldGroups( HTREEITEM _hItem )
{
	HTREEITEM familyItem = _hItem;
	HTREEITEM childItem;

	while( familyItem )
	{
		GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(familyItem);
		ASSERT( info );

		const char* familyName = info->family;

		FamilyGroups::FamilyMap::iterator famIt = s_FamilyGroups.familyMap.find( familyName );
		if( famIt == s_FamilyGroups.familyMap.end() )
		{
			delete info;

			childItem = m_treeView.GetChildItem(familyItem);
			while( childItem )
			{
				GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(childItem);
				ASSERT( info );
				delete info;

				childItem = m_treeView.GetNextSiblingItem(childItem);
			}

			HTREEITEM deleteItem = familyItem;
			familyItem = m_treeView.GetNextSiblingItem(familyItem);
			m_treeView.DeleteItem( deleteItem );
			continue;
		}
		else
		{
			childItem = m_treeView.GetChildItem(familyItem);
			while( childItem )
			{
				GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(childItem);
				ASSERT( info );

				const char* childname = info->object;

				if( famIt->second.find(childname) == famIt->second.end() )
				{
					delete info;

					HTREEITEM nextChild = m_treeView.GetNextSiblingItem( childItem );
					m_treeView.DeleteItem( childItem );
					childItem = nextChild;
					continue;
				}

				childItem = m_treeView.GetNextSiblingItem(childItem);
			}
		}
				
		familyItem = m_treeView.GetNextSiblingItem(familyItem);
	}

	return false;
}

//-----------------------------------------------------------------------------------------------------

bool CGroupBar::deleteData( HTREEITEM _hItem )
{
	if( !_hItem )
		return true;

	GroupInfo* info = (GroupInfo*)m_treeView.GetItemData(_hItem);
	if( info )
	{
		delete info;
		m_treeView.SetItemData(_hItem,NULL);
	}

	if( m_treeView.ItemHasChildren(_hItem) )
	{
		HTREEITEM child = m_treeView.GetChildItem(_hItem);
		while( child )
		{
			deleteData( child );
			child = m_treeView.GetNextSiblingItem(child);
		}
	}

	return deleteData( m_treeView.GetNextSiblingItem(_hItem) );
}
