//--------------------------------------------------------------------------//
//                                                                          //
//                               dbtreeview.cpp    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Viewer/Docuview/treeview.cpp 2     4/22/03 11:36a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------

#include <windows.h>
#include <commctrl.h> 
#include <commdlg.h> 

#include "TreeView.h"

#include <malloc.h>


//--------------------------------------------------------------------------

bool TreeView_CreateBranchByParsing( HWND _hTree, const char* _label, char* _parser, DWORD _data )
{
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

		hItem = TreeView_InsertBranch( _hTree, pLabel, hItem );

		pLabel = pLabelNext;
		if( pLabel )
			pLabelNext = strstr(pLabel, _parser);
	}

	if( hItem != TVI_ROOT )
	{
		TVITEM item;
		item.mask = TVIF_HANDLE | TVIF_PARAM;
		item.hItem = hItem;
		item.state = 0;
		item.stateMask = 0;
		item.pszText = 0;
		item.cchTextMax = 0;
		item.iImage = 0;
		item.iSelectedImage = 0;
		item.cChildren = 0;
		item.lParam = _data;

		TreeView_SetItem( _hTree, &item );
	}

	return false;
}

//--------------------------------------------------------------------------

HTREEITEM TreeView_InsertBranch( HWND _hTree, char* _label, HTREEITEM _item )
{
	HTREEITEM hItem = _item;

	TVITEM item;
	memset( &item, 0, sizeof(item) );
	char buffer[128];

	if( hItem == TVI_ROOT )
	{
		hItem = TreeView_GetRoot( _hTree );
	}
	else
	{
		hItem = TreeView_GetNextItem( _hTree, hItem, TVGN_CHILD );
	}

	while( hItem != NULL )
	{
		memset( &item, 0, sizeof(item) );
		item.mask = TVIF_HANDLE | TVIF_TEXT;
		item.hItem = hItem;
		item.pszText = buffer;
		item.cchTextMax = 128;
		
		TreeView_GetItem( _hTree, &item );

		if( !strcmp(buffer,_label) )
		{
			return hItem;
		}

		hItem = TreeView_GetNextSibling( _hTree, hItem );
	}

	TVINSERTSTRUCT insert;
	
	item.mask = TVIF_TEXT | TVIF_PARAM;
	item.pszText = _label;
	item.cchTextMax = 128;
	item.lParam = (DWORD)-1;

	memset( &insert, 0, sizeof(insert) );
	memcpy( &insert.item, &item, sizeof(item) );

	insert.hParent = _item;
	insert.hInsertAfter = TVI_SORT;

	hItem = TreeView_InsertItem( _hTree, &insert );

	return hItem;
}
/*

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
*/