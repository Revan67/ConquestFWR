/**************************************************************
* CSplashWnd.hpp : A very simple splash screen class
*
* Chris N. Haddan
* May 98
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CSPLASHWND_H
#define __CSPLASHWND_H

#define CSPLASHWND_CLASS "CSplashWndClass"
#include "windows.h"

class CSplashWnd
{
	public:
		CSplashWnd();
		~CSplashWnd();
		Create (HINSTANCE hInst, HWND hwndParent, UINT id);
		LRESULT CALLBACK CSplashWnd::HandleSplashMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		HDC m_hdc;
		HWND m_hWnd;
		int m_iTimer;
		int m_iTimerLen;
		HBITMAP m_hbm;
		HBITMAP m_oldhbm;
		HINSTANCE m_hInst;
};

#endif