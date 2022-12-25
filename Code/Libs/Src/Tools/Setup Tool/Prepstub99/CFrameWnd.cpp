/**************************************************************
* CFrameWnd.cpp: A simple window frame handler class
*
* Chris N. Haddan
* May 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CFrameWnd.hpp"
#include "commctrl.h"
#include "malloc.h"
#include "math.h"

#define CFRAME_WND_CLASS "aas:CFrameWndClass"


CFrameWnd::CFrameWnd ()
{
	m_hWndFrame = NULL;
	m_cFrames = 0;
	m_cMaxFrames = 0;
	m_iFrame = 0;
	m_papFrames = NULL;
}


CFrameWnd::~CFrameWnd()
{
	if (m_hWndFrame)
		DestroyWindow (m_hWndFrame);

	for (int iFrame=0; iFrame < m_cFrames; iFrame++)
	{
		free (m_papFrames[iFrame]);
		m_papFrames[iFrame]=NULL;
	}
	free (m_papFrames);
	m_papFrames=NULL;

	m_hWndFrame = NULL;
}


bool CFrameWnd::Create (HINSTANCE hInst, HWND hwndParent, int x, int y, int width, int height, bool bHorizontal)
{
	InitCommonControls ();
	
	WNDCLASSEX wc;

	// register the CListViewFrame window classes

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, CFRAME_WND_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) CFrameWndWndFrameProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = NULL;; 
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = CFRAME_WND_CLASS; 

		if (!RegisterClassEx (&wc))
			return false;
	}

	m_iFrame = 0;
	m_bHorizontal = bHorizontal;

	m_hWndFrame = CreateWindowEx (0L, CFRAME_WND_CLASS, "",
			WS_CHILD, 0, 0, 0, 0,
			hwndParent, (HMENU) this, hInst, NULL);

	if (m_hWndFrame == NULL) 
		return false;

	SetWindowLong (m_hWndFrame, GWL_USERDATA, (long)this);

	ShowWindow (m_hWndFrame, SW_SHOW);

	return true;
}


bool CFrameWnd::AddFrame (bool bFixed, double iSize)
{
	if (m_cFrames >= m_cMaxFrames)
	{
		m_cMaxFrames  += FRAME_ALLOC;

		if (m_cFrames == 0)
		{
			m_papFrames = (FRAMEDATA **) malloc (sizeof (FRAMEDATA *) * m_cMaxFrames);
		}
		else
		{
			m_papFrames = (FRAMEDATA **) realloc (m_papFrames, sizeof (FRAMEDATA *) * m_cMaxFrames);
		}

		if (m_papFrames == NULL) return false;
	}

	m_papFrames[m_cFrames] = (FRAMEDATA *) malloc (sizeof (FRAMEDATA));
	m_papFrames[m_cFrames]->bFixed = bFixed;
	m_papFrames[m_cFrames]->iSize = iSize;

	m_cFrames += 1;

	return true;
}


bool CFrameWnd::AttachFrame (HWND hwnd, int iFrameNumber)
{
	// add to list

	if (iFrameNumber > m_cFrames - 1)
	{
		return false;
	}
	
	m_papFrames[iFrameNumber]->hwnd = hwnd;
	SetParent (hwnd, m_hWndFrame); 
	return true;
}


bool CFrameWnd::Resize (int x, int y, int width, int height)
{
	SetWindowPos (m_hWndFrame, HWND_TOP, x, y, width, height, SWP_SHOWWINDOW);
	return true;
}


LRESULT CALLBACK CFrameWndWndFrameProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CFrameWnd *pFrameView = (CFrameWnd *)GetMenu (hWnd);

	return (pFrameView->HandleFrameMessages (hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK CFrameWnd::HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int iFrame;

	switch (msg)
	{
		case WM_ERASEBKGND:
			return 1;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				if (m_hWndFrame)
					MoveWindow (m_hWndFrame, 0, 0, LOWORD (lParam), HIWORD (lParam), true);
				
				// resize the children

				// bugbug: only supports horizontal frames currently.

				if (m_papFrames)
				{
					int iTotalSize = 0;
			
					// add up the static sized frames
					for (iFrame=0; iFrame < m_cFrames; iFrame++)
					{
						if (m_papFrames[iFrame]->bFixed)
							iTotalSize += (int)m_papFrames[iFrame]->iSize;
					}

					int iPos = 0;

					// resize the windows
					for (iFrame=0; iFrame < m_cFrames; iFrame++)
					{
						if (m_papFrames[iFrame]->bFixed)
						{
							MoveWindow (
								m_papFrames[iFrame]->hwnd, 
								0, iPos, 
								LOWORD (lParam), 
								(int)m_papFrames[iFrame]->iSize, 
								true);

							iPos += (int)m_papFrames[iFrame]->iSize;
						}
						else
						{
							MoveWindow (
								m_papFrames[iFrame]->hwnd, 
								0, iPos, 
								LOWORD (lParam), 
								(int)ceil((HIWORD (lParam) - iTotalSize) * (m_papFrames[iFrame]->iSize)), 
								true);

							iPos += (int)ceil((HIWORD (lParam) - iTotalSize) * (m_papFrames[iFrame]->iSize));
						}
					}
				}
			} 
			break;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}
