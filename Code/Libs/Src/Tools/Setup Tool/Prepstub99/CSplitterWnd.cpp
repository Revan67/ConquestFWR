/**************************************************************
* CSplitterWnd.cpp: a window splitter class
*
* Chris N. Haddan
* April 23rd, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CSplitterWnd.hpp"
#include "resource.h"
#include "string.h"
#include "math.h"


CSplitterWnd::CSplitterWnd ()
{
	m_hInst =		NULL;
	m_hWndParent =	NULL;
	m_hWndFrame =	NULL;
	m_hWndSplitter = NULL;
	m_hWndPanelA =	NULL;
	m_hWndPanelB =	NULL;

	m_nThickness = 6;

	m_bHorizontal = true;
	m_bDragging = false;
	m_bFullWindowDrag = true;

	m_ptDrag.x = 0;
	m_ptDrag.y = 0;

	m_ofs.x = 0;
	m_ofs.y = 0;
}


CSplitterWnd::~CSplitterWnd ()
{
	m_hWndFrame = NULL;
	m_hWndSplitter = NULL;
	m_hWndPanelA = NULL;
	m_hWndPanelB = NULL;
	m_hWndParent = NULL;
	m_hInst = NULL;
	
	m_bHorizontal = true;
	m_bDragging = false;
}


bool CSplitterWnd::Create (HINSTANCE hInst, HWND hwndParent, RECT rect, int nPercent, bool bHorizontal)
{
	WNDCLASSEX wc;
	
	m_hInst = hInst;
	m_bHorizontal = bHorizontal;
	m_nPercent = ((double)nPercent * (double)0.01);

	// register the CSplitterWnd window classes

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, SPLITTER_FRAME_CLASS, &wc))
	{ 
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) SplitterWndFrameProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor (NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = SPLITTER_FRAME_CLASS; 

		if (!RegisterClassEx (&wc))
			return false;
	}

	ZeroMemory (&wc, sizeof (wc));
	wc.cbSize = sizeof (WNDCLASSEX);

	if (!GetClassInfoEx (hInst, SPLITTER_SLIDER_CLASS, &wc))
	{
		ZeroMemory (&wc, sizeof (wc));
		wc.cbSize = sizeof (WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC) SplitterWndSliderProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInst;
		wc.hIcon = LoadIcon (hInst, MAKEINTRESOURCE (IDI_PREPSTUB98ICON)); 
		wc.hCursor = NULL;
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
		wc.lpszMenuName = NULL;
		wc.lpszClassName = SPLITTER_SLIDER_CLASS; 
		
		if (!RegisterClassEx (&wc))
			return false;
	}
	
	// create the container frame that holds the splitter, and 2 panels

	m_hWndFrame = CreateWindowEx(0, SPLITTER_FRAME_CLASS, "Frame", 
			WS_CHILD , 
			0,0,0,0,
			hwndParent, (HMENU) this, NULL, NULL);    

	if (m_hWndFrame == NULL) return false;

	// create the splitter 

	m_hWndSplitter = CreateWindowEx( 0, SPLITTER_SLIDER_CLASS, "Splitter", 
			WS_CHILD, 
			0, 0, 0, 0,
			m_hWndFrame, (HMENU) this, NULL, NULL);    

	if (m_hWndSplitter == NULL) return false;
	
	ResizePanels(rect.right-rect.left, rect.bottom-rect.top);
	
	ResizeSplitter();	

	ShowWindow (m_hWndFrame, SW_SHOW);
	ShowWindow (m_hWndSplitter, SW_SHOW);

	return true;
}


void CSplitterWnd::ResizeSplitter()
{
	int x, y, dx, dy;
	RECT rect;

	if (m_hWndFrame == NULL) return;

	GetClientRect (m_hWndFrame, &rect);

	if (m_bHorizontal)
	{
		x = 0;
		y = (int) ((double)(rect.bottom-rect.top) * (m_nPercent));
		dx = rect.right-rect.left;
		dy = m_nThickness;
	}
	else
	{
		x = (int) ((double)(rect.right-rect.left) * (m_nPercent));
		y = 0;
		dx = m_nThickness;
		dy = rect.bottom-rect.top;
	}

	MoveWindow (m_hWndSplitter, x, y, dx, dy, true);
}


void CSplitterWnd::Resize (int x, int y, int dx, int dy)
{
	MoveWindow (m_hWndFrame, x, y, dx, dy, true);
}


LRESULT CALLBACK SplitterWndFrameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSplitterWnd *pSplitter = (CSplitterWnd *)GetMenu (hWnd);

	return (pSplitter->HandleFrameMessages (hWnd, msg, wParam, lParam));
}


LRESULT CALLBACK SplitterWndSliderProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CSplitterWnd *pSplitter = (CSplitterWnd *)GetMenu (hWnd);

	return (pSplitter->HandleSliderMessages (hWnd, msg, wParam, lParam));
}


void CSplitterWnd::MapCoordinates (POINTS *pts)
{
	POINT pt; 
	POINTS pts2;

	pt.x = 0;
	pt.y = 0;

	ClientToScreen (m_hWndSplitter, &pt);
	
	pt.x += pts->x - m_ofs.x;
	pt.y += pts->y - m_ofs.y;

	if (m_bHorizontal)
	{
		pt.y += (int) (0.5 * m_nThickness);
	}
	else
	{
		pt.x += (int) (0.5 * m_nThickness);
	}

	ScreenToClient (m_hWndFrame, &pt);

	pts2.x = LOWORD(POINTTOPOINTS(pt));
	pts2.y = HIWORD(POINTTOPOINTS(pt));

	pts->x = pts2.x;
	pts->y = pts2.y;
}


LRESULT CALLBACK CSplitterWnd::HandleSliderMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	POINTS pts;
	int x, y;

	switch (msg)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				RECT rect;
				HDC hdc = BeginPaint (hWnd, &ps);
			
				GetClientRect (hWnd, &rect);
		
				FillRect (hdc, &rect, (HBRUSH)(COLOR_3DFACE+1));
				DrawEdge(hdc, &rect, EDGE_RAISED ,  BF_RECT);

				EndPaint (hWnd, &ps);
			}
			break;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;

		case WM_SETCURSOR:
			{
				HCURSOR hCursor;

				if (m_bHorizontal)
				{
					hCursor = LoadCursor (m_hInst, MAKEINTRESOURCE(IDC_SPLITH));
				}
				else
				{
					hCursor = LoadCursor (m_hInst, MAKEINTRESOURCE(IDC_SPLITV));
				}
			
				SetCursor (hCursor);

				return 1;
			}
			break;
		case WM_LBUTTONDOWN:

			m_bDragging = true;

			SetCapture (m_hWndSplitter);

			pts = MAKEPOINTS (lParam);

			m_ofs.x = pts.x;
			m_ofs.y = pts.y;

			MapCoordinates (&pts);

			DrawSplitter (pts);

			m_ptDrag.x = pts.x;
			m_ptDrag.y = pts.y;
			
			return 0;

		case WM_LBUTTONUP:
			if (m_bDragging)
			{
				RECT rect;

				ReleaseCapture(); 

				pts = MAKEPOINTS (lParam);
				MapCoordinates (&pts);

				GetClientRect (m_hWndFrame, &rect);

				if (pts.x < (int)(m_nThickness * 0.5))  pts.x = (int)(m_nThickness * 0.5);
				if (pts.y < (int)(m_nThickness * 0.5))  pts.y = (int)(m_nThickness * 0.5);

				if (pts.x + (int)(m_nThickness * 0.5) > rect.right-rect.left)  pts.x = rect.right-rect.left - (int)(m_nThickness * 0.5);
				if (pts.y + (int)(m_nThickness * 0.5) > rect.bottom-rect.top)  pts.y = rect.bottom-rect.top - (int)(m_nThickness * 0.5);

				if (m_bHorizontal)
				{
					y = pts.y - m_nThickness/2;
					m_nPercent = ((double)(y) / (double)(rect.bottom-rect.top));
				}
				else
				{
					x = pts.x - m_nThickness/2;
					m_nPercent = ((double)(x) / (double)(rect.right-rect.left));
				}

				DrawSplitter (pts);
			
				ResizePanels(rect.right - rect.left, rect.bottom - rect.top);

				ResizeSplitter();

				m_bDragging = false;
				
				return 0;
			}

		case WM_MOUSEMOVE:
			
			if (m_bDragging)
			{	
				RECT rect;

				DrawSplitter (m_ptDrag);

				pts = MAKEPOINTS (lParam);
				MapCoordinates (&pts);

				if (pts.x < (int)(m_nThickness * 0.5))  pts.x = (int)(m_nThickness * 0.5);
				if (pts.y < (int)(m_nThickness * 0.5))  pts.y = (int)(m_nThickness * 0.5);


				GetClientRect (m_hWndFrame, &rect);
				if (pts.x + (int)(m_nThickness * 0.5) > rect.right-rect.left)  pts.x = rect.right-rect.left - (int)(m_nThickness * 0.5);
				if (pts.y + (int)(m_nThickness * 0.5) > rect.bottom-rect.top)  pts.y = rect.bottom-rect.top - (int)(m_nThickness * 0.5);
				

				m_ptDrag.x = pts.x;
				m_ptDrag.y = pts.y;

				DrawSplitter (m_ptDrag);

				if (m_bFullWindowDrag)
				{
					// begin realtime - window resize
					if (m_bHorizontal)
					{
						y = pts.y - m_nThickness/2;
						m_nPercent = ((double)(y) / (double)(rect.bottom-rect.top));
					}
					else
					{
						x = pts.x - m_nThickness/2;
						m_nPercent = ((double)(x) / (double)(rect.right-rect.left));
					}

					ResizePanels(rect.right - rect.left, rect.bottom - rect.top);
					ResizeSplitter();
					// end realtime - window resize
				}

			}
			return 0;
	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}

LRESULT CALLBACK CSplitterWnd::HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_ERASEBKGND:
			return 1;

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
			{
				ResizePanels(LOWORD (lParam), HIWORD (lParam));
				ResizeSplitter();
			}
			break;

		case WM_DESTROY:
			PostQuitMessage (0);
			return 0;

	}
	return (DefWindowProc (hWnd, msg, wParam, lParam));
}


void CSplitterWnd::ResizePanels (int nWidth, int nHeight)
{
	int nPanel_x;
	int nPanel_y;
	int nPanel_dx;
	int nPanel_dy;
	int nSlider_pos;

	if (m_bHorizontal)
	{
		nSlider_pos = (int) floor ((double)nHeight * m_nPercent);
	}
	else
	{
		nSlider_pos = (int) floor ((double)nWidth * m_nPercent);
	}

	if (m_hWndPanelA)
	{
		if (m_bHorizontal)
		{
			nPanel_x = 0;
			nPanel_y = 0;
			nPanel_dx = nWidth;
			nPanel_dy = nSlider_pos;
		}
		else
		{
			nPanel_x = 0;
			nPanel_y = 0;
			nPanel_dx = nSlider_pos;
			nPanel_dy = nHeight;
		}

		MoveWindow (m_hWndPanelA, nPanel_x, nPanel_y, nPanel_dx, nPanel_dy, true);
		InvalidateRect (m_hWndPanelA, NULL, true);
	}

	if (m_hWndPanelB)
	{
		if (m_bHorizontal)
		{
			nPanel_x = 0;
			nPanel_y = nSlider_pos + m_nThickness;
			nPanel_dx = nWidth;
			nPanel_dy = nHeight - nPanel_y;
		}
		else
		{
			nPanel_x = nSlider_pos + m_nThickness;
			nPanel_y = 0;
			nPanel_dx = nWidth - nPanel_x;
			nPanel_dy = nHeight;
		}

		MoveWindow (m_hWndPanelB, nPanel_x, nPanel_y, nPanel_dx, nPanel_dy, true);
		InvalidateRect (m_hWndPanelB, NULL, true);
	}

	if (m_hWndSplitter)
		InvalidateRect (m_hWndSplitter, NULL, true);
}


HWND CSplitterWnd::GetHwnd()
{
	return m_hWndFrame;
}


void CSplitterWnd::DrawSplitter (POINTS pt)
{
	RECT rect;
	HPEN oldPen;
	HPEN newPen;
	HDC hdc;
	int oldRop;

	if (m_bFullWindowDrag)
		return;

	hdc = GetDC (m_hWndFrame );

	newPen = CreatePen (PS_SOLID, m_nThickness, RGB (255,255,255));

	oldPen = (HPEN) SelectObject (hdc, newPen);

	oldRop = SetROP2 (hdc, R2_XORPEN);

	GetClientRect (m_hWndFrame, &rect);
	
	if (m_bHorizontal)
	{
		MoveToEx (hdc, rect.left, pt.y,  NULL);
		LineTo (hdc, rect.right, pt.y);
	}
	else
	{
		MoveToEx (hdc, pt.x, 0, NULL);
		LineTo (hdc, pt.x, rect.bottom);
	}

	SelectObject (hdc, oldPen);
	DeleteObject (newPen);
	SetROP2 (hdc, oldRop);
	ReleaseDC (GetParent (m_hWndFrame), hdc);
}


bool CSplitterWnd::AttachPanel (HWND hWnd, int nPanel)
{
	RECT rect;

	if (hWnd == NULL)
		return false;

	switch (nPanel)
	{
		case 0:
			m_hWndPanelA = hWnd;
			SetParent (m_hWndPanelA, m_hWndFrame);
		break;
		case 1:
			m_hWndPanelB = hWnd;
			SetParent (m_hWndPanelB, m_hWndFrame);
		break;
		default:
			return false;
	}

	GetClientRect (m_hWndFrame, &rect);
	ResizePanels(rect.right-rect.left, rect.bottom-rect.top);
	return true;
}