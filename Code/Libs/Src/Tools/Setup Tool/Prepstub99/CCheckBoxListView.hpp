/***************************************************************************
* CCheckBoxListView.hpp : Derived ListView class that supports Checkboxes.
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
****************************************************************************/
#pragma once
#ifndef __CCHECKBOXLISTVIEW_H
#define __CCHECKBOXLISTVIEW_H

#include "windows.h"
#include "CListView.hpp"

#define COLOR_GRAD 2
#define COLOR_VAR 8

class CCheckBoxListView : public CListView
{
	public:
		CCheckBoxListView (): CListView () 
		{ 
			m_nRowHeight = 22;
			m_dwExStyle =0;
			m_dwStyle = LVS_OWNERDRAWFIXED | LVS_NOCOLUMNHEADER;
			HDC hdc = GetDC (NULL);
			
			m_rgbHiLiteTextColor	= RGB (255,255,255);
			m_rgbHiLiteBkColor		= RGB (0,0,0);

			m_rgbBkColor			= GetSysColor (COLOR_BTNFACE);
			m_rgbTextColor			= GetSysColor (COLOR_BTNTEXT);

			m_font = CreateFont (-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |FF_DONTCARE, "Tahoma");
			ReleaseDC (NULL, hdc);
			m_bSelectable = false;  //we will handle all button clicks in ProcessMessages
		}

		~CCheckBoxListView()
		{
			if (m_font)
				DeleteObject (m_font);
		}
		virtual COLORREF GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem);
		virtual	COLORREF GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem);
		virtual bool SpanColumns(LPDRAWITEMSTRUCT lpDrawItem);
		virtual void DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem);
		virtual void DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem);

	private:
		virtual LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual void GetText (int iCol, LPARAM lParam, char *szString);
};


void AddCheckBoxImageList(HWND hWnd);
void RemoveCheckBoxImageList(HWND hWnd);


#endif