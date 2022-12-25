/**************************************************************
* CSplitterWnd.hpp: A window splitter class
*
* Chris N. Haddan
* April 23rd, 1998
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#pragma once
#ifndef __CSPLITTERWND_H
#define __CSPLITTERWND_H

#include "windows.h"

#define CS_HORIZONTAL	true
#define CS_VERTICAL		false

#define SPLITTER_FRAME_CLASS "CSplitterWndClass_Frame"
#define SPLITTER_SLIDER_CLASS "CSplitterWndClass_Slider"

class CSplitterWnd
{
	public:
		CSplitterWnd();
		~CSplitterWnd();
		bool Create (HINSTANCE hInst, HWND hwndParent, RECT rect, int nPercent, bool bHorizontal);
		bool AttachPanel (HWND hWnd, int nPanel);

		void MapCoordinates (POINTS *pts);
		void ResizeSplitter();
		void DrawSplitter (POINTS pt);
		void ResizePanels (int nWidth, int nHeight);
		void Resize (int x, int y, int dx, int dy);

		HWND GetHwnd();		

		LRESULT CALLBACK HandleSliderMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK HandleFrameMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		HINSTANCE m_hInst;

		bool m_bHorizontal;
		bool m_bDragging;
		bool m_bFullWindowDrag;

		POINTS m_ptDrag;
		POINTS m_ofs;

		HWND m_hWndFrame;
		HWND m_hWndSplitter;
		HWND m_hWndPanelA;
		HWND m_hWndPanelB;
		HWND m_hWndParent;

		int m_nThickness;
		double m_nPercent;
};

LRESULT CALLBACK SplitterWndFrameProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SplitterWndSliderProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#endif