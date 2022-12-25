/**************************************************************
* CListView.hpp: a list view class
*
* Chris N. Haddan
* April 22nd, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __CLISTVIEW_H
#define __CLISTVIEW_H

#include "windows.h"
#include "commctrl.h"

#define	 CLISTVIEW_FRAME_CLASS "aas:CListViewFrameClass"

#ifdef STRICT
#define SCWNDPROC WNDPROC
#else
#define SCWNDPROC FARPROC
#endif

class CListView
{
	public:
		CListView();
		~CListView();
		virtual int GetItemWidth (int iCol, int iRow);
		void AutoSizeColumn (int iCol);
		bool Create (HINSTANCE hInst, HWND hwndParent);
		bool Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height);
		bool CreateImageList ();
		bool AddColumn (char *szLabel);
		bool AddColumn (char *szLabel, int iWidth);
		bool Resize (int x, int y, int width, int height);
		void Refresh ();
		bool AddItem (void *pvItem);
		void PaintListViewVoidAreas (HDC hdc);
		int	ListView_GetActualColumnWidth (int iCol);
		
		
		void DrawItemColumn(HDC hdc, LPTSTR lpsz, LPRECT prcClip, int nFont);
		void DrawItemColumn(HDC hdc, LPTSTR lpsz, LPRECT prcClip, HFONT hFont);
		BOOL CalcStringEllipsis(HDC hdc, LPTSTR lpszString, int cchMax, UINT uColWidth);

		BOOL DeleteAllItems ();
		HWND GetHwnd() { return m_hWndFrame; };
	
		HWND GetLvHwnd() { return m_hWnd; };
		LRESULT CALLBACK HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		virtual LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;
		void Delete();

		virtual void DrawListViewItem(LPDRAWITEMSTRUCT lpDrawItem);
		virtual void DrawItem (int iItem, HDC hdc, LPRECT prcClip, LPDRAWITEMSTRUCT lpDrawItem);
		virtual void GetText (int iCol, LPARAM lParam, char *szString) = 0;
		virtual bool SpanColumns(LPDRAWITEMSTRUCT lpDrawItem);
		virtual COLORREF GetRowTextColor (LPDRAWITEMSTRUCT lpDrawItem);
		virtual	COLORREF GetRowBkColor (LPDRAWITEMSTRUCT lpDrawItem);
		long GetPreviousWndProcAddr()
		{
			return m_lPreviousWndProc;
		}

		COLORREF GetBkColor ();
		COLORREF GetTextColor ();
		COLORREF GetHiLiteTextColor ();
		COLORREF GetHiLiteBkColor ();
		HFONT GetFont ();
		int GetRowHeight();
		void SetSelectable (bool bSelectable)
			{	m_bSelectable = bSelectable; }
		bool GetSelectable ()
			{	return (m_bSelectable); }
	protected:
		HINSTANCE m_hInst;		// app instance
		HWND	m_hWndFrame;	// parent frame for listview to receive notify msg's
		HWND	m_hWnd;			// list view window handle
		DWORD	m_dwExStyle;
		DWORD	m_dwStyle;
		int		m_cColumns;
		long	m_lPreviousWndProc;
		COLORREF m_rgbBkColor;
		COLORREF m_rgbTextColor;
		COLORREF m_rgbHiLiteTextColor;
		COLORREF m_rgbHiLiteBkColor;
		HFONT	m_font;
		int		m_nRowHeight;
		bool	m_bSelectable;
};

LRESULT CALLBACK CListViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
CListView *GetObjectPtrFromHwnd (HWND hWnd);
LRESULT CALLBACK ListViewWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
