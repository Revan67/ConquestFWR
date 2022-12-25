//--------------------------------------------------------------------------//
//                                                                          //
//                             EulaWin.cpp                             //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Author: Tmauer $

    $Header: /Conquest/App/Src/EulaWin.cpp 9     8/16/02 4:58p Tmauer $
*/			    
//---------------------------------------------------------------------------
/*
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include "pch.h"
#include "stdio.h"
#include <globals.h>
#include <richedit.h>
#include "UserDefaults.h"

#include "resource.h"

FILE * eulaFile = NULL;

char * regestryPath = NULL;
char * eulaFileName = NULL;

DWORD CALLBACK eulaEditStreamCallback(
  DWORD dwCookie, // application-defined value
  LPBYTE pbBuff,  // pointer to a buffer
  LONG cb,        // number of bytes to read or write
  LONG *pcb       // pointer to number of bytes transferred
)
{
	if(eulaFile)
	{
		*pcb = fread(pbBuff,1,cb,eulaFile);
		return false;
	}
	*pcb = 0;
	return false;
}

wchar_t acceptButton[64];
wchar_t declineButton[64];
HWND acceptHandle;
HWND declineHandle;
WNDPROC buttonProc;
HFONT buttonFont;

LRESULT CALLBACK buttonCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT retVal = CallWindowProc(buttonProc, hwndDlg, uMsg, wParam, lParam);
	if(uMsg == WM_PAINT)
	{
		wchar_t * buffer = NULL;
		if(acceptHandle == hwndDlg)
			buffer = acceptButton;
		else
			buffer = declineButton;
		RECT winRect;
		GetClientRect(hwndDlg,&winRect);
		int x = winRect.left+((winRect.right-winRect.left)/2);

		HDC hDC = GetDC(hwndDlg);
		SelectObject(hDC,buttonFont);
		SIZE size;
		GetTextExtentPoint32W(hDC,buffer,wcslen(buffer),&size);
		int y = winRect.top+((winRect.bottom-winRect.top)/2) + (size.cy/2) - 2;
		SetTextAlign(hDC,TA_CENTER|TA_BASELINE);
		SetBkMode(hDC, TRANSPARENT);
		TextOutW(hDC,x,y,buffer,wcslen(buffer));
		ReleaseDC(hwndDlg,hDC);
	};
	return retVal;
};


BOOL CALLBACK eulaDialogCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			wchar_t buffer[255];
			wcsncpy(buffer, _localLoadStringW(IDS_EULA_TITLE), sizeof(buffer)/sizeof(wchar_t));
			char bufferC[255];
			_localWideToAnsi(buffer, bufferC, sizeof(bufferC));
			SetWindowText(hwndDlg,bufferC);

			acceptHandle = GetDlgItem(hwndDlg,IDOK);
			wcsncpy(acceptButton, _localLoadStringW(IDS_EULA_ACCEPT), sizeof(acceptButton)/sizeof(wchar_t));
			SetWindowText(acceptHandle," ");
			SetFocus(acceptHandle);
			buttonProc = (WNDPROC)(GetWindowLong(acceptHandle,GWL_WNDPROC));
			SetWindowLong(acceptHandle,GWL_WNDPROC,(LONG)(&buttonCallback));
			
			declineHandle = GetDlgItem(hwndDlg,IDCANCEL);
			wcsncpy(declineButton, _localLoadStringW(IDS_EULA_DECLINE), sizeof(declineButton)/sizeof(wchar_t));
			SetWindowText(declineHandle," ");
			SetWindowLong(declineHandle,GWL_WNDPROC,(LONG)(&buttonCallback));

			eulaFile = fopen(eulaFileName,"rb");
			if(eulaFile)
			{
				EDITSTREAM streamDef;
				streamDef.dwCookie = 0;
				streamDef.dwError = 0;
				streamDef.pfnCallback = eulaEditStreamCallback;
				SendDlgItemMessage(hwndDlg,IDC_EULA_TEXT,EM_STREAMIN,SF_RTF,(DWORD)(&streamDef));
				fclose(eulaFile);
			}
			HDC hDC = GetDC(acceptHandle);
			if(findUserLang() == MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL))
			{
				buttonFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,CHINESEBIG5_CHARSET ,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,NULL);
				if(!buttonFont)
					buttonFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,CHINESEBIG5_CHARSET ,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"MingLiu");
			}
			else if(findUserLang() == MAKELANGID(LANG_KOREAN,SUBLANG_KOREAN))
			{
				buttonFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"굴림");
				if(!buttonFont)
					buttonFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Gulim");

			}
			if(!buttonFont)
				buttonFont = CreateFont(-MulDiv(10, GetDeviceCaps(hDC, LOGPIXELSY), 72),0,0,0,FW_NORMAL,false,false,false,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,"Arial");	
			ReleaseDC(acceptHandle,hDC);
			return true;
			break;
		}
	case WM_COMMAND:
		{
            if (wParam == IDOK)
            { 
				U32 data = 1;
				DEFAULTS->SetDataInRegistry("eula",&data,sizeof(data));
                EndDialog(hwndDlg, true); 
                return true; 
            } 
			else if(wParam == IDCANCEL)      
			{
				DeleteObject(buttonFont);
				buttonFont = NULL;
                EndDialog(hwndDlg, false); 
                return true; 
			}
			break;
		}
	}
	return false;
}
 
bool DoEulaWin(char * eulaFile)
{
	U32 data = 0;
	bool bGotData = (DEFAULTS->GetDataFromRegistry("eula",&data,sizeof(data)) != 0);
	if(!bGotData || (bGotData && !data))
	{
		eulaFileName = eulaFile;
		int retVal = DialogBox(hResource,MAKEINTRESOURCE(IDD_EULA_DIALOG),NULL,eulaDialogCallback) != 0;
		if(retVal == -1)
			return true;
		return (retVal != 0);
	}
	return true;
}
//----------------------------------------------------------------------------
//--------------------------End EulaWin.cpp------------------------------
//----------------------------------------------------------------------------
