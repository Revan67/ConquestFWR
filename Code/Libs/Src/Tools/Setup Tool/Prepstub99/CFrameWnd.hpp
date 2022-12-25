/**************************************************************
* CFrameWnd.hpp: A simple frame window class
*
* Chris N. Haddan
* May 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __CFRAMEWND_H
#define __CFRAMEWND_H

#include "windows.h"

#define FRAME_ALLOC 3;

typedef struct tagFRAMEDATA
{
	HWND hwnd;		// handle to panel
	bool bFixed;
	double iSize;
	RECT rect;		// size and location
} FRAMEDATA;


class CFrameWnd 
{
	public:
		CFrameWnd ();
		~CFrameWnd ();
		bool Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height, bool bHorizontal);
		HWND GetHwnd() { return m_hWndFrame; }
		bool AttachFrame (HWND hwnd, int iFrameNumber);
		bool AddFrame (bool bFixed, double iSize);
		bool SetFrame (int iFrame);
		bool Resize (int x, int y, int width, int height);
		void PreSize ();
		LRESULT CALLBACK HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		bool		m_bHorizontal;
		HWND		m_hWndFrame;
		int			m_cFrames;
		int			m_cMaxFrames;
		int			m_iFrame;
		FRAMEDATA	**m_papFrames;
};


LRESULT CALLBACK CFrameWndWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


#endif