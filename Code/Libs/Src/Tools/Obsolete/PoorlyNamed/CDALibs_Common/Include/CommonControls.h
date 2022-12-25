// CommonControls.h
//
//
//


#ifndef COMMONCONTROLS_H
#define COMMONCONTROLS_H


#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

// Handy function
//

inline char *MakeString( char *sz, const char *fmt, ... )
{
	va_list args;
	va_start(args, fmt);
	vsprintf( sz, fmt, args );
	va_end(args);
	return sz;
}


// List View
//

// 

inline U32 LV_InsertRow( HWND hLV, const char *main_text, U32 param, ... )
{
	LV_ITEM lvi = { LVIF_TEXT|LVIF_PARAM, 0, 0, 0, 0, (char*)main_text, 0, 0, param };
	U32 col = 0;

	U32 item = ListView_InsertItem( hLV, &lvi );
	U32 sw = 16+ListView_GetStringWidth( hLV, lvi.pszText );	
	U32 cw = ListView_GetColumnWidth( hLV, col );			
	ListView_SetColumnWidth( hLV, col, __max(sw,cw) );	
	col++;

	va_list va;
	va_start(va,param);

	char *txt;
	while( (txt=va_arg(va,char*)) ) {
		lvi.mask = LVIF_TEXT; 
		lvi.iItem = item;	
		lvi.iSubItem = col;
		lvi.pszText = txt;
		ListView_SetItem( hLV, &lvi );
		sw = 16+ListView_GetStringWidth( hLV, lvi.pszText );	
		cw = ListView_GetColumnWidth( hLV, col );			
		ListView_SetColumnWidth( hLV, col, __max(sw,cw) );	
		col++;
	}
	va_end(va);

	return item;
}

//

inline U32 LV_SetColumns( HWND hLV, const char *main_text, U32 align, ... )
{
	LV_COLUMN lvi = { LVCF_TEXT|LVCF_FMT, align, 0, (char*)main_text, 0, 0 };
	U32 col = 0;

	U32 item = ListView_InsertColumn( hLV, col, &lvi );
	U32 sw = 16+ListView_GetStringWidth( hLV, lvi.pszText );	
	U32 cw = ListView_GetColumnWidth( hLV, col );			
	ListView_SetColumnWidth( hLV, col, __max(sw,cw) );	
	col++;

	va_list va;
	va_start(va,align);

	char *txt;
	while( (txt=va_arg(va,char*)) ) {
		lvi.pszText = txt;
		lvi.fmt = va_arg(va,U32);
		U32 item = ListView_InsertColumn( hLV, col, &lvi );
		sw = 16+ListView_GetStringWidth( hLV, lvi.pszText );	
		cw = ListView_GetColumnWidth( hLV, col );			
		ListView_SetColumnWidth( hLV, col, __max(sw,cw) );	
		col++;
	}
	va_end(va);

	return item;
}

//

inline U32 LV_GetFirstSelectedData( HWND hLV )
{
	U32 num_items = ListView_GetItemCount( hLV );
	for( U32 i=0; i<num_items; i++ ) {
		if( ListView_GetItemState( hLV, i, LVIS_SELECTED ) & LVIS_SELECTED ) {
			LV_ITEM lvi = { LVIF_PARAM, i, 0, 0, 0, NULL, 0, 0, 0 };
			ListView_GetItem( hLV, &lvi );
			return lvi.lParam;
		}
	}
	return 0;
}

//

inline void LV_Clear( HWND hLV )
{
	ListView_DeleteAllItems(hLV);
}

//

#endif
