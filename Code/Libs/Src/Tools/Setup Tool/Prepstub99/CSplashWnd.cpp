/**************************************************************
* CSplashWnd.cpp : A very simple splash screen class
*
* Chris N. Haddan
* May 98
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#define COMPILE_MULTIMON_STUBS

#include "CSplashWnd.hpp"
#include "multimon.h"



CSplashWnd::CSplashWnd ()
{
	m_hdc = NULL;
	m_hWnd = NULL;
	m_iTimer = 0;
	m_iTimerLen = 2500;
	m_hbm = NULL;
	m_oldhbm = NULL;
	m_hInst = NULL;
}


CSplashWnd::~CSplashWnd ()
{
	if (m_oldhbm)
	{
		SelectObject (m_hdc, m_oldhbm);
	}

	if (m_hbm)
	{
		DeleteObject (m_hbm);
	}
	
	if (m_hdc)
		DeleteDC (m_hdc);

	m_hInst = NULL;
	m_hWnd = NULL;
}


LRESULT CALLBACK CSplashWndWinProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSplashWnd *pSplash = (CSplashWnd *)GetWindowLong (hWnd, GWL_USERDATA);

	return (pSplash->HandleSplashMessages (hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK CSplashWnd::HandleSplashMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	BITMAP bm;
	switch (msg)
	{
		case WM_PAINT:
			{
				HDC hdc = BeginPaint (hWnd, &ps);

				GetObject (m_hbm, sizeof (BITMAP), &bm);

				BitBlt (hdc, 0, 0, bm.bmWidth, bm.bmHeight, m_hdc, 0, 0, SRCCOPY);
			
				EndPaint (hWnd, &ps);
				return 0;
			}
		case WM_ERASEBKGND:
			return 1;
		case WM_TIMER:
			KillTimer (hWnd, m_iTimer);
			DestroyWindow (hWnd);
			delete this;
			return 0;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}


void CenterWindowOnMonitor (HWND hWndParent, HWND hWnd)
{    
	HMONITOR hMonitor;    
	MONITORINFO mi;

	RECT rectMonitor;
	RECT rectWindow;

	GetWindowRect(hWnd, &rectWindow);

    hMonitor = MonitorFromWindow(hWndParent, MONITOR_DEFAULTTONEAREST);

    mi.cbSize = sizeof(mi);    
	GetMonitorInfo(hMonitor, &mi);
    rectMonitor  = mi.rcMonitor;

	rectWindow.left = rectMonitor.left + ((rectMonitor.right - rectMonitor.left) - (rectWindow.right - rectWindow.left)) /2;
	rectWindow.top = rectMonitor.top + ((rectMonitor.bottom - rectMonitor.top) - (rectWindow.bottom - rectWindow.top)) /2;

    SetWindowPos(hWnd, NULL, rectWindow.left, rectWindow.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


CSplashWnd::Create (HINSTANCE hInst, HWND hwndParent, UINT id)
{
	WNDCLASSEX wc;
	BITMAP bm;
	RECT rect;

	m_hInst = hInst;

	m_hbm = LoadBitmap (m_hInst, MAKEINTRESOURCE(id));

	if (!m_hbm)
		return false;

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, CSPLASHWND_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) CSplashWndWinProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = NULL;; 
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CSPLASHWND_CLASS; 

		if (!RegisterClassEx (&wc))
			return false;
	}

	GetWindowRect (GetDesktopWindow(), &rect);
	GetObject (m_hbm, sizeof (BITMAP), &bm);

	m_hWnd = CreateWindowEx (WS_EX_TOPMOST,
				CSPLASHWND_CLASS, 
				"CSplashWnd",
				WS_POPUPWINDOW, 
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				bm.bmWidth + 2, 
				bm.bmHeight + 2,
				hwndParent, NULL, hInst, NULL);

	if (m_hWnd == NULL) 
		return false;

	CenterWindowOnMonitor (hwndParent, m_hWnd);
	SetWindowLong (m_hWnd, GWL_USERDATA, (long)this);

	m_hdc = CreateCompatibleDC (NULL);
	SelectObject (m_hdc, m_hbm);
	
	ShowWindow (m_hWnd, SW_SHOW);
	UpdateWindow (m_hWnd);
	SetFocus (hwndParent);

	m_iTimer = SetTimer (m_hWnd, 1, m_iTimerLen, NULL);

	// if can't get a timer, show the splash for 1 sec, then kill the window here.
	if (m_iTimer == 0)
	{
		Sleep (1000);
		DestroyWindow (m_hWnd);
	}
	return true;
}
