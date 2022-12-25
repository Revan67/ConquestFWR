//--------------------------------------------------------------------------//
//                                                                          //
//                               dbtreeview.h    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Viewer/Docuview/treeview.h 2     4/22/03 11:36a Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//
#ifndef _DB_TREE_VIEW_HEADER_H_
#define _DB_TREE_VIEW_HEADER_H_

HTREEITEM TreeView_InsertBranch( HWND _hTree, char* _label, HTREEITEM _item );

bool TreeView_CreateBranchByParsing( HWND _hTree, const char* _label, char* _parser, DWORD _data );

#endif
