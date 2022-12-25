//--------------------------------------------------------------------------//
//                                                                          //
//                                Splash.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 2000 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Header: /Conquest/App/Src/Splash.cpp 3     11/15/00 10:18a Jasony $
*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>
#include "CQTrace.h"
#include "Resource.h"

#include <HeapObj.h>
#include <FileSys.h>
#include <EventSys.h>
#include <TSmartPointer.h>

#define DLG_WIDTH   640
#define DLG_HEIGHT  480
//--------------------------------------------------------------------------//
//
static void resizeDlg (HWND hwnd, HWND hPicture)
{
	const S32 desktop_w = GetSystemMetrics(SM_CXSCREEN);
	const S32 desktop_h = GetSystemMetrics(SM_CYSCREEN);

	S32 x = (desktop_w - DLG_WIDTH) / 2;
	S32 y = (desktop_h - DLG_HEIGHT) / 2;

	::SetWindowPos(hwnd, HWND_TOP, x, y, DLG_WIDTH, DLG_HEIGHT, 0);
	::SetWindowPos(hPicture, HWND_TOP, 0, 0, DLG_WIDTH, DLG_HEIGHT, 0);
}
//--------------------------------------------------------------------------//
//
static HANDLE loadbitmap (void)
{
	HANDLE handle;
	HDC hdc = GetDC(0);
	S32 bpp;
	//
	// Get # of bits per pixel
	//
	
	bpp = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

	ReleaseDC(0, hdc);

	if (bpp < 16)
		handle = LoadImage(NULL, _localLoadString(IDS_SPLASH_LOWRES), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	else
		handle = LoadImage(NULL, _localLoadString(IDS_SPLASH_HIGHRES), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	return handle;
}
//--------------------------------------------------------------------------//
//
static BOOL CALLBACK splashDlgProc (HWND hwnd, UINT message, UINT wParam, LONG lParam)
{
	BOOL result=0;

	switch (message)
	{
	case WM_INITDIALOG:
		{
			HWND hItem = GetDlgItem(hwnd, IDC_PICTURE1);
			resizeDlg(hwnd, hItem);
			SetWindowText(hwnd, _localLoadString(IDS_APP_NAME));
			HANDLE hBitmap = loadbitmap();
			SendMessage(hItem, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) hBitmap);
			ShowWindow(hwnd, SW_SHOW);
			result = 1;  // system should set focus of control
		}
		break;

	}  // end switch (message)


	return result;
}
//--------------------------------------------------------------------------//
//
void __stdcall ShutdownSplash (HWND hwnd)
{
	if (hwnd)
	{
		HWND hItem = GetDlgItem(hwnd, IDC_PICTURE1);
		HGDIOBJ hBitmap = (HGDIOBJ) SendMessage(hItem, STM_SETIMAGE, (WPARAM) IMAGE_BITMAP, (LPARAM) 0);

		DestroyWindow(hwnd);

		if (hBitmap)
			DeleteObject(hBitmap);
	}
}
//--------------------------------------------------------------------------//
//
HWND __stdcall DoSplashScreen (void)
{
	HWND result;
	MSG msg;

	result = CreateDialogParam(hResource, MAKEINTRESOURCE(IDD_DIALOG9), hMainWindow, splashDlgProc, NULL);

	//
	// Serve message queue
	//
	
	while (PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage(&msg, NULL, 0, 0 ))
		{
			exit(msg.wParam);	// pass exit code back to exit chain
		}
		
		if (IsDialogMessage(msg.hwnd, &msg)==0)
		{
			TranslateMessage(&msg); 
			DispatchMessage(&msg);
		}
	}

	return result;
}





//------------------------------------------------------------------------------//
//--------------------------------End Splash.cpp--------------------------------//
//------------------------------------------------------------------------------//

