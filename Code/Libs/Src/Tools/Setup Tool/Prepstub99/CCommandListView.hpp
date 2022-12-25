/***********************************************************************
* CCommandListView.hpp : Derived ListView class for command list
*
* Chris N. Haddan
* April 1998
*
* (C) 1998 Microsoft Corporation
*
************************************************************************/
#pragma once
#ifndef __CCOMMANDLISTVIEW_H
#define __CCOMMANDLISTVIEW_H

#include "windows.h"
#include "CListView.hpp"

#define COLOR_GRAD 2
#define COLOR_VAR 8

class CCommandListView : public CListView
{
	public:
		CCommandListView (): CListView () 
		{ 
			m_dwExStyle =0;
			m_dwStyle = LVS_OWNERDRAWFIXED;
			HDC hdc = GetDC (NULL);
			
			m_rgbHiLiteTextColor	= RGB (255,255,255);
			m_rgbHiLiteBkColor		= RGB (0,0,0);
			m_hImageList			= NULL;

			m_font = CreateFont (-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |FF_DONTCARE, "Tahoma");
			m_boldfont = CreateFont (-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |FF_DONTCARE, "Tahoma");
			m_italicfont = CreateFont (-MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72), 0, 0, 0, FW_BLACK, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH |FF_DONTCARE, "Courier New");
			ReleaseDC (NULL, hdc);
			m_nRowHeight = 21;
		}

		~CCommandListView ()
		{
			DeleteObject (m_font);
			DeleteObject (m_boldfont);
			DeleteObject (m_italicfont);

			m_font			= NULL;
			m_boldfont		= NULL;
			m_italicfont	= NULL;
			m_hImageList	= NULL;
		}
		void RemoveImageList();
		void AddImageList();
		virtual COLORREF GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem);
		virtual	COLORREF GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem);
		virtual bool SpanColumns(LPDRAWITEMSTRUCT lpDrawItem);
		virtual void DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem);
		virtual void DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem);
		virtual LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual void GetText (int iCol, LPARAM lParam, char *szString);
		virtual int GetItemWidth (int iCol, int iRow);
	private:
		HFONT m_boldfont;
		HFONT m_italicfont;
		HIMAGELIST m_hImageList;
};


#endif