/**************************************************************
* CTabView.hpp: A tab view control
*
* Chris N. Haddan
* May 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __CTABVIEW_H
#define __CTABVIEW_H

#include "windows.h"

#define TAB_ALLOC 3;

typedef struct tagTABDATA
{
	HWND hwnd;		// handle to panel
	RECT rect;		// size and location
} TABDATA;


class CTabView 
{
	public:
		CTabView ();
		~CTabView ();
		bool CTabView::Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height);
		bool CTabView::AddTab (char *szText);
		HWND GetHwnd() { return m_hWndFrame; }
		bool AttachTab (HWND hwnd, int iTabNumber);
		bool SetTab (int iTab);
		bool Resize (int x, int y, int width, int height);
		void PreSize ();
		LRESULT CALLBACK HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		HWND		m_hWnd;
		HWND		m_hWndFrame;
		HFONT		m_hFont;
		int			m_cTabs;
		int			m_cMaxTabs;
		int			m_iTab;
		TABDATA	 ** m_papTabs;
};


LRESULT CALLBACK CTabViewWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#endif