//--------------------------------------------------------------------------//
//                                                                          //
//                               dbtreeview.cpp    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbtreeview.cpp 7     11/14/03 2:54p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------

#include <Afxcmn.h>
#include <Afxtempl.h>
#include <malloc.h>

#include "dbTreeView.h"

//--------------------------------------------------------------------------

#define APSTUDIO_INVOKED
	#include "resource.h"
	#define IDD_TREE_DLG _APS_NEXT_RESOURCE_VALUE
#undef APSTUDIO_INVOKED

extern HWND hMainWindow;

//-------------------------------------------------------------------------------------

#ifdef _DEBUG
#ifndef DEBUG_NEW
	void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine);
	#define DEBUG_NEW new(THIS_FILE, __LINE__)
	#if _MSC_VER >= 1200
		void __cdecl operator delete(void* p, LPCSTR lpszFileName, int nLine);
	#endif
#endif
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//--------------------------------------------------------------------------

DbTreeView::DbTreeView()
{
	m_TreeCtrl = NULL;
}

//--------------------------------------------------------------------------

DbTreeView::~DbTreeView()
{
	delete m_TreeCtrl;
	m_TreeCtrl = NULL;
}

//--------------------------------------------------------------------------

bool DbTreeView::Init( HWND _hwnd )
{
	RECT r;
	WINDOWPLACEMENT winPlace;

	::GetClientRect( _hwnd, &r );
	::ShowWindow( _hwnd, SW_HIDE );
	::GetWindowPlacement( _hwnd, &winPlace );

	CWnd* cWnd = new CWnd();
	cWnd->Attach( ::GetParent(_hwnd) );

	DWORD dwStyle = TVS_LINESATROOT | TVS_HASLINES | TVS_NOTOOLTIPS | WS_CHILD | WS_BORDER | TVS_HASBUTTONS;
	
	m_TreeCtrl = new CTreeCtrl();
	m_TreeCtrl->Create( dwStyle, r, cWnd, IDD_TREE_DLG );


	m_TreeCtrl->ShowWindow( SW_NORMAL );
	m_TreeCtrl->SetWindowPos( NULL, winPlace.rcNormalPosition.left, winPlace.rcNormalPosition.top, 0, 0, SWP_NOSIZE );

	return true;
}

//--------------------------------------------------------------------------

bool DbTreeView::CreateBranch( DWORD _data, char* _label, ... )
{
	if( !m_TreeCtrl ) return false;

	HTREEITEM item = TVI_ROOT;

	char* label = _label;

	va_list marker;
	va_start( marker, _label );
	while( label != NULL )
	{
		item  = InsertBranch( label, item );
		label = va_arg( marker, char*);
	}
	va_end( marker );

	if( item != TVI_ROOT )
	{
		return( m_TreeCtrl->SetItemData(item,_data) != false );
	}

	return false;
}

//--------------------------------------------------------------------------

bool DbTreeView::Clear()
{
	if( !m_TreeCtrl ) return false;
	return( m_TreeCtrl->DeleteAllItems() != false );
}

//--------------------------------------------------------------------------

bool DbTreeView::CreateBranchByParsing( char* _label, char* _parser, DWORD _data )
{
	if( !m_TreeCtrl ) return false;

	HTREEITEM hItem = TVI_ROOT;

	char* pBuffer = (char*) _alloca( strlen(_label) + 1 );
	strcpy( pBuffer, _label );

	char* pLabel = pBuffer;
	char* pLabelNext = strstr(pLabel, _parser);

	while( pLabel )
	{
		if( pLabelNext )
		{
			*pLabelNext = 0;
			pLabelNext += strlen(_parser);
		}

		hItem = InsertBranch( pLabel, hItem );

		pLabel = pLabelNext;
		if( pLabel )
			pLabelNext = strstr(pLabel, _parser);
	}

	if( hItem != TVI_ROOT )
	{
		return( m_TreeCtrl->SetItemData(hItem,_data) != false );
	}

	return false;
}

//--------------------------------------------------------------------------

bool DbTreeView::RenameLabel( char* _oldLabel, char* _newLabel )
{
	if( !m_TreeCtrl ) return false;

	return 0;
}

//--------------------------------------------------------------------------

bool DbTreeView::OnNotify( HWND _hwnd, WPARAM _wParam, LPARAM _lParam )
{
	if( !m_TreeCtrl ) return false;

	int idCtrl   = (int) _wParam; 
	LPNMHDR pnmh = (LPNMHDR) _lParam; 

	switch( pnmh->code )
	{
		case NM_DBLCLK:
		{
			LPNMHDR lpnmh = (LPNMHDR)_lParam; 
			PostMessage(hMainWindow,WM_COMMAND,MAKEWPARAM(ID_EDIT,0),0);
			break;
		}
		case NM_RETURN:
		{
			// why is this never thown?
			PostMessage(hMainWindow,WM_COMMAND,MAKEWPARAM(ID_EDIT,0),0);
			return 0;
		}
		case TVN_SELCHANGED:
		{
			LPNMTREEVIEW tv = (LPNMTREEVIEW)pnmh;
			break;
		}
		case TVN_ITEMEXPANDING:
		{
			LPNMTREEVIEW lpnmtv = (LPNMTREEVIEW)_lParam;
			m_TreeCtrl->SortChildren( lpnmtv->itemNew.hItem );
			break;
		}
		case NM_RCLICK:
		{
			HTREEITEM hCurItem = m_TreeCtrl->GetDropHilightItem();
			if (hCurItem == NULL)
			{
				hCurItem = m_TreeCtrl->GetSelectedItem();
				if (hCurItem == NULL)
				{
					return false;
				}
			}

			// setting up the location for the pop-up menu
			m_TreeCtrl->Select(hCurItem, TVGN_CARET);

			RECT rectSel;
			m_TreeCtrl->GetItemRect(hCurItem, &rectSel, TRUE);
			m_TreeCtrl->ClientToScreen(&rectSel);

			rectSel.top  += 10;
			rectSel.left += 10;
			 
			// Load the menu resource. 	
			CMenu menu;
			menu.LoadMenu(IDR_FILEPOPUP);

			// Display the shortcut menu. Track the right mouse button. 

			CMenu* pSubMenu = menu.GetSubMenu(0); 

			BOOL ret = pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, rectSel.left, rectSel.top, m_TreeCtrl); 

			if( ret == IDM_UNDOCHECKOUT )
			{
				ret++;
			}
			else if( ret == IDM_CHECKOUT )
			{
				ret++;
			}
			else if( ret == IDM_CHECKIN )
			{
				ret++;
			}
		}
	}

	if( pnmh->code == TVN_KEYDOWN )
	{
		LPNMTVKEYDOWN ptvkd = (LPNMTVKEYDOWN) _lParam;

		switch( ptvkd->wVKey )
		{
			case VK_DELETE:
				PostMessage(hMainWindow, WM_COMMAND, MAKELONG(IDC_DELETE, 0), (LONG)_hwnd);
				return 0;
				
			case VK_INSERT:
				PostMessage(hMainWindow, WM_COMMAND, MAKELONG(IDC_CLONE, 0), (LONG)_hwnd);
				return 0;

			case VK_SPACE:
				PostMessage(hMainWindow,WM_COMMAND,MAKEWPARAM(ID_EDIT,0),0);
				return 0;
		}
	}
	return 0;
}

//--------------------------------------------------------------------------

int DbTreeView::GetCurrentSelectionData( void )
{
	if( !m_TreeCtrl ) return false;

	HTREEITEM hitem = m_TreeCtrl->GetSelectedItem();

	if( !hitem )
	{
		return -1;
	}

	return (int)m_TreeCtrl->GetItemData( hitem );
}

//--------------------------------------------------------------------------

bool DbTreeView::GetRootToSelected( char* _rootLabel, int _rootLabelMax, char* _seperator )
{
	_rootLabel[0] = 0;

	if( !m_TreeCtrl ) return false;

	HTREEITEM hitem = m_TreeCtrl->GetSelectedItem();

	if( !hitem )
		return false;

	CArray<CString,CString&> stringArray;

	if( m_TreeCtrl->GetItemData(hitem) != 0xFFFFFFFF )
	{
		hitem = m_TreeCtrl->GetParentItem( hitem );
	}

	while( hitem )
	{
		stringArray.Add( m_TreeCtrl->GetItemText(hitem) );
		hitem = m_TreeCtrl->GetParentItem( hitem );
	}

	for( int i = stringArray.GetSize()-1; i >= 0; i-- )
	{
		strcat( _rootLabel, (const char*)stringArray[i] );
		strcat( _rootLabel, _seperator );
	}
	
	return 0;
}

//--------------------------------------------------------------------------

void DbTreeView::SavePlace()
{
	m_CurrentItemName[0] = 0;

	if( m_TreeCtrl )
	{
		CArray<CString,CString&> stringArray;
		HTREEITEM item = m_TreeCtrl->GetSelectedItem();
		while( item )
		{
			stringArray.Add( m_TreeCtrl->GetItemText(item) );
			item = m_TreeCtrl->GetParentItem( item );
		}

		for( int i = stringArray.GetSize()-1; i >= 0; i-- )
		{
			strcat( m_CurrentItemName, (const char*)stringArray[i] );

			if( i )
				strcat( m_CurrentItemName, "!!" );
		}
	}
}

//--------------------------------------------------------------------------

HTREEITEM DbTreeView::FindPlace( HTREEITEM _item, const char* _archname, int _archindex )
{
	if( !_item )
	{
		return NULL;
	}

	CString mask = _archname;
	mask = mask.Left( _archindex );
	
	int nPrev = mask.Find("!!",0);
	while( nPrev > 0 )
	{
		mask = mask.Mid( nPrev + 2 );
		nPrev = mask.Find("!!",0);
	}

	HTREEITEM item = _item;

	while( item )
	{
		CString label = m_TreeCtrl->GetItemText(item);
		if( label == mask )
		{
			break;
		}
		item = m_TreeCtrl->GetNextSiblingItem(item);
	}

	if( item && _archindex < (int)strlen(_archname) )
	{
		mask = _archname;
		int nextIndex = mask.Find("!!", _archindex + 2);

		if( nextIndex < 0 )
		{
			nextIndex = mask.GetLength();
		}
		else
			mask = mask.Left( _archindex + nextIndex );

		return FindPlace( m_TreeCtrl->GetChildItem(item), _archname, nextIndex );
	}

	return item;
}

//--------------------------------------------------------------------------

void DbTreeView::RestorePlace()
{
	if( m_TreeCtrl )
	{
		CString string = m_CurrentItemName;
		int index = string.Find("!!",0);

		if( index != -1 )
		{
			string = string.Left( index );
			HTREEITEM item = FindPlace( m_TreeCtrl->GetRootItem(), m_CurrentItemName, index );
			if( item )
			{
				m_TreeCtrl->SelectSetFirstVisible( item );
			}
		}
	}
}

//--------------------------------------------------------------------------

HTREEITEM DbTreeView::InsertBranch( char* _label, HTREEITEM _item )
{
	HTREEITEM hItem = _item;

	if( hItem == TVI_ROOT )
	{
		hItem = m_TreeCtrl->GetNextItem( hItem, TVGN_ROOT );
	}
	else
	{
		hItem = m_TreeCtrl->GetNextItem( _item, TVGN_CHILD );
	}

	while( hItem != NULL )
	{
		CString s = m_TreeCtrl->GetItemText( hItem );

		if( s.Compare(_label)==0 )
		{
			return hItem;
		}

		hItem = m_TreeCtrl->GetNextSiblingItem( hItem );
	}
	
	hItem = m_TreeCtrl->InsertItem( _label, _item );
	m_TreeCtrl->SetItemData( hItem, (DWORD)-1 );

	return hItem;
}
