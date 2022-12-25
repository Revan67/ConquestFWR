//--------------------------------------------------------------------------//
//                                                                          //
//                               dbtreeview.h    							//
//                                                                          //
//                  COPYRIGHT (C) 2003 Fever Pitch Studios, INC.            //
//                                                                          //
//--------------------------------------------------------------------------//
/*
	$Header: /Libs/Src/Tools/adb/dbtreeview.h 3     7/28/03 3:14p Ajackson $

	$Author: Ajackson $
*/
//--------------------------------------------------------------------------//
#ifndef _DB_TREE_VIEW_HEADER_H_
#define _DB_TREE_VIEW_HEADER_H_

class DbTreeView
{
	private:
		class CTreeCtrl* m_TreeCtrl;
		char  m_CurrentItemName[512];

	protected:
		DbTreeView( const& DbTreeView ){}

		HTREEITEM InsertBranch( char* _label, HTREEITEM _item );

		HTREEITEM FindPlace( HTREEITEM _item, const char* _archname, int _archindex );

	public:
		DbTreeView();

		virtual ~DbTreeView();

		bool Init( HWND _hwnd );

		bool CreateBranch( DWORD _data, char* _label, ... );

		bool CreateBranchByParsing( char* _label, char* _parser, DWORD _data );

		bool RenameLabel( char* _oldLabel, char* _newLabel );

		bool OnNotify( HWND _hwnd, WPARAM _wParam, LPARAM _lParam );

		bool Clear();

		int GetCurrentSelectionData( void );

		bool GetRootToSelected( char* _rootLabel, int _rootLabelMax, char* _seperator );

		void SavePlace();

		void RestorePlace();

		inline IsValid( void )
		{
			return( m_TreeCtrl != NULL );
		}
};

#endif
