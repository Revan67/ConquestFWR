/*
    print.cpp - Copyright (c) Microsoft Corp. 1991-1998

    Created by Craig Henry (stolen from the SQL Group, originally written by Gavin Jancke

*/

#include <time.h>
#include <stdio.h>
#include "windows.h"
#include "hotsetup.h"
#include "setup.h"
#include <string.h>
#include "stdlib.h"
#include "print.h"
#include "hotsetuprc.h"
#include "imectrl.h"

#ifdef NTWIN32
#define LockData(a)
#define UnlockData(a)
#define MYDEVMODE   CONST LPDEVMODEA
#endif

using namespace NGLOBALS;

Printer *g_pPrint = NULL;
/*@Method
   This method is called by a window to setup the printer.  It is callable from
the main menu of the application, and takes a window pointer `pwin`.  It puts
up the printer setup dialog and causes windows to change it\'s printer setup.
*/
boolean Printer::PrintSetup ()
{
    PD.lStructSize = sizeof (PRINTDLG);
    PD.hwndOwner = hwndParent;
    PD.Flags = PD_PRINTSETUP;
    
    BOOL bRet = PrintDlg (&PD);
    return bRet ? TRUE/*OK BUTTON*/ : FALSE/*CANCEL or ERROR*/;
}

/*@Method
   This method returns the PD structure.
*/

void *Printer::ReturnPDStruct()
{
return ((void *) &PD);
}

/*@Method
   This method starts the printing process for the caller.  It will start printing
for most of the supported modes, MDI, DIALOG, DEVMDI.  The parameters are
`Conn` the connection used by the caller to fill the data (this method gets the
server name from the connection), `pWindow` pointer to the calling window, `subTitle`
subtitle for the printout, and `wMode` type of printing occuring ('PRINT_DIALOG',
'PRINT_DEVMDI','PRINT_MDI').
*/
boolean Printer::PrintStart ()
{
	DOCINFO di;
	ZeroMemory(&di, sizeof(DOCINFO));
	di.cbSize = sizeof(DOCINFO);
	di.lpszDocName = GetSetupTitle();
    lpfnAbortPrinterProc = NULL, lpfnAbortProc = NULL; // procs for callbacks

    wMaxFileLength = 0;         // reset internal variables
    CurrentLine = 1;
    bAbort = FALSE;
    pszTitle = GetSetupTitle();
    PD.hwndOwner = hwndParent; // save window handle
    hPr = (HDC)GetPrintDC ();       // get printer display context
    if (! hPr)
        return FALSE;
    // make proc instances for callbacks
    lpfnAbortPrinterProc = (ABORTPROC)MakeProcInstance ( (FARPROC)AbortProc, GetResourceInst());
    lpfnAbortProc = (DLGPROC)MakeProcInstance ((FARPROC)AbortDlgProc, GetResourceInst());
    if (! lpfnAbortPrinterProc || ! lpfnAbortProc)
    {
		if(GetPrinterFont())
		{
			SelectObject(hPr,GetOldFont());
			DeleteObject(GetPrinterFont());
			SetOldFont(NULL);
			SetPrinterFont(NULL);
		}
        DeleteDC (hPr);
        return FALSE;
    }
    // setup the abort handler with the printer calls of windows
    SetAbortProc(hPr, lpfnAbortPrinterProc);
    // create the print about dialog (modeless).
    hwndAbortDlg = CreateDialog (GetResourceInst(), "ABORTPRINT", PD.hwndOwner, lpfnAbortProc);

	if (hwndAbortDlg)
	{
		//
		//Ensure that IME is disabled for the Abort Print dialog...
		//
		changeIMEStatus(hwndAbortDlg, SCF_IME_DISABLE);
	}

    // start the printing
    if(StartDoc(hPr, &di) <=0 || ((IOStatus = StartPage(hPr)) <= 0)|| bAbort)
    {
        // error, clean up.
        DestroyAbortWnd ();
		if(GetPrinterFont())
		{
			SelectObject(hPr,GetOldFont());
			DeleteObject(GetPrinterFont());
			SetOldFont(NULL);
			SetPrinterFont(NULL);
		}
        DeleteDC (hPr);
        FreeProcInstance (lpfnAbortPrinterProc);
        FreeProcInstance (lpfnAbortProc);
        return FALSE;
    }
    IOStatus = StartPage (hPr);

    // get metrics for printing
    GetTextMetrics (hPr, &TextMetric);
    nPageSize = GetDeviceCaps (hPr, VERTRES);
    LineSpace = TextMetric.tmHeight + TextMetric.tmExternalLeading;
	if(LineSpace == 0)
		LinesPerPage = 54;
	else
        LinesPerPage = nPageSize / LineSpace - 1;
    // create a print out header
    return TRUE;
}
/*@Method
   This method stops the printing process.  The method cleans up all
allocated memory, releases teh print DC and other good stuff.
*/
boolean Printer::PrintStop ()
{
    if (IOStatus >= 0 && ! bAbort)  // if not an abort, then stop print
    {
        EndPage(hPr);
        EndDoc (hPr);
        DestroyAbortWnd ();
    }

    DeleteDC (hPr);
//    FreeProcInstance (lpfnAbortPrinterProc);
//    FreeProcInstance (lpfnAbortProc);
    return TRUE;
}
/*@Method
   This method will print the line of text `pszText` out to the printer, this
call is only valid after the printing has been started and is used to print
out a line at a time.
*/
boolean Printer::PrintText (char *pszText)
{
    TabbedTextOut (hPr, TextMetric.tmMaxCharWidth, CurrentLine * LineSpace, (LPSTR) pszText, (pszText) ? lstrlen (pszText) : 0, 0, (LPINT)NULL, 0);
    CurrentLine += 2;
    return TRUE;
}

boolean Printer::PrintMultiLine(char *ptext)
{  // Actually does the printing, sending the printer escape in place
   // of crlf's embedded in the text.
   
   // MEMO : Mar.12,1998 03:18 by yutaka.
   // We can not accept CRLF-Space-CRLF sequence to the Japanese EULA.TXT
   // printing. Japanese string should be CRLF at the right edge of the page.
   //
   // for IsJapan()
   //
   LONG nPrevHeight = 0;
   LONG nPrevLength = 0;
   LONG nPageWidth = GetDeviceCaps( hPr, HORZRES );
   LONG nLMargine = TextMetric.tmMaxCharWidth*2;
   LONG nRMargine = TextMetric.tmMaxCharWidth;
   
   if (ptext) {
      while (*ptext != '\0')
      {
	     char *ptr2=ptext,*ptr=ptext,*startptr=ptext,*printptr;
		 int length;
         while( (ptr  = CharNext(ptr2)) && *ptr != '\0')
		 {
			 ptr2 = ptr;
			 if(*ptr == 0x0d || *ptr == 0X0a)
			 {
                length = ptr - ptext;
				printptr = ptext;
				if(*(ptr +1) == 0x0a)
				   ptext += length+2;
				else
				   ptext += length+1;
				break;
			 }
			 if ( IsJapan() ){ // Mar.27,1998 01:24 by yutaka.
			 	// CAUTION : NOT SUPPORTING the ENGLISH WORD BREAK.
			 	
				length = ptr - ptext;
				printptr = ptext;
				
				char *printString = new char[length+1];
				strncpy( printString, printptr, length );
				printString[length] = '\0';
				
				RECT r;
				r.left = r.top = r.bottom = 0;
				r.right = nPageWidth - nLMargine - nRMargine;
				r.bottom = DrawText( hPr, (LPSTR) printString, length, &r, 
					DT_CALCRECT | 
					DT_WORDBREAK|DT_EXPANDTABS|DT_LEFT|DT_NOPREFIX );
				delete printString;
				
				if ( nPrevHeight && r.bottom != nPrevHeight ){ // 2 lines ?
					length = nPrevLength;
					ptext += length;
					break;
				}
				nPrevHeight = r.bottom;
				nPrevLength = length;
			 }
		 }
		 if(ptext == startptr)
		 {
              ptr = ptext + ((ptext) ? lstrlen (ptext) : 0);
			  ptr2 = ptr;
			  printptr = ptext;
              length = ptr - ptext;
			  ptext = ptr;
		 }
         char *printString = new char[length + 1];
		 if ( IsJapan() ){ // Mar.27,1998 01:25 by yutaka.
			  strncpy( printString, printptr, length );
			  printString[length] = '\0';
			  
			  RECT r;
			  r.left = nLMargine;
			  r.top = CurrentLine * LineSpace;
			  r.right = nPageWidth;// - nRMargine;
			  r.bottom = CurrentLine * LineSpace + nPrevHeight;
			  nPrevHeight = 0;
			  nPrevLength = 0;
			  DrawText( hPr, (LPSTR) printString, length, &r, 
				  DT_WORDBREAK|DT_EXPANDTABS|DT_LEFT|DT_NOPREFIX );
		 } else {
            CharToOemBuff (printptr,printString,length+1);
            TabbedTextOut (hPr, TextMetric.tmMaxCharWidth, CurrentLine * LineSpace, (LPSTR) printString, length, 0, (LPINT)NULL, 0);
		 }
         delete printString;
         if (++CurrentLine > LinesPerPage)
         {
			IOStatus = EndPage(hPr);
            CurrentLine = 1;
            IOStatus = StartPage (hPr);
            if (IOStatus < 0 || bAbort)
                break;
         }
      }
   }
   return TRUE;
}

/*@Method
   This method fills the PD structure inside the object with information needed
to actually print.  A display context is also created for printing to.  This routine
is called internally only.
*/
HDC Printer::GetPrintDC (void)
{
    LPDEVMODE lpDevMode;
    LPDEVNAMES lpDevNames;

    PD.lStructSize = sizeof (PRINTDLG);
    if (! PD.hDevNames)    /* Retrieve default printer if none selected.*/
    {
        PD.Flags = PD_RETURNDEFAULT | PD_RETURNDC;
        PrintDlg (&PD);
    }
    if (! PD.hDevNames)
        return 0;

    lpDevNames = (LPDEVNAMES) GlobalLock (PD.hDevNames);
    if (PD.hDevMode)
        lpDevMode = (LPDEVMODE) GlobalLock (PD.hDevMode);
    else
        lpDevMode = NULL;
    PD.hDC = (HDC)CreateDC ((LPSTR) lpDevNames + lpDevNames->wDriverOffset,
    (LPSTR) lpDevNames + lpDevNames->wDeviceOffset,
    (LPSTR) lpDevNames + lpDevNames->wOutputOffset,
    (LPDEVMODE) lpDevMode);

    GlobalUnlock (PD.hDevNames);
	{
	   char font[150];
	   EBULoadString(GetResourceInst(),STR_PRINTERFONTTYPE,font,150);
	   if ( IsJapan() ){ // Mar.27,1998 01:27 by yutaka.
			char *p = "RESERR [id:";
			if ( strncmp( font, p, lstrlen(p) ) == 0 ){
				lstrcpy( font, "" ); // clear the illegal facename.
			}
	   }
	   SetPrinterFont(CreateFont(0,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
								IsJapan() ? DEFAULT_CHARSET : OEM_CHARSET, // Mar.27,1998 01:29 by yutaka.
								OUT_DEFAULT_PRECIS,
								CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,VARIABLE_PITCH | FF_ROMAN,
								font));
		LOGFONT lf;
		GetObject(GetPrinterFont(),sizeof(LOGFONT), &lf);
		// map to printer font metrics
		HDC hDCFrom = GetDC(NULL);
		if(lf.lfHeight == 0)
		   lf.lfHeight = -MulDiv(10, GetDeviceCaps(hDCFrom, LOGPIXELSY), 72);
		if(lf.lfWidth == 0)
		   lf.lfWidth = -MulDiv(10, GetDeviceCaps(hDCFrom, LOGPIXELSX), 200);
 
		lf.lfHeight = MulDiv(lf.lfHeight, GetDeviceCaps(PD.hDC,LOGPIXELSY),
			GetDeviceCaps(hDCFrom, LOGPIXELSY));
		lf.lfWidth = MulDiv(lf.lfWidth, GetDeviceCaps(PD.hDC,LOGPIXELSX),
			GetDeviceCaps(hDCFrom, LOGPIXELSX));
		ReleaseDC(NULL, hDCFrom);

		// create it, if it fails we just use the printer's default.
		DeleteObject(GetPrinterFont());
		SetPrinterFont(CreateFontIndirect(&lf));
	    if(GetPrinterFont())
		{
		  	   SetOldFont(SelectObject(PD.hDC,GetPrinterFont()));
		}	   
	}
    if (PD.hDevMode)
        GlobalUnlock (PD.hDevMode);
    return PD.hDC;
}
/*@Method
   This method is a message loop called by a C routine which is in turn
called by windows.  It processes messages for the abort print dialog box.
This should NEVER be called by anyone internally.  The parameters are
standard windows dialog message handler parameters.
*/
int Printer::abortDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HMENU hSysMenu;

    switch (msg)
        {
        case WM_COMMAND :
            bAbort = TRUE;
            DestroyAbortWnd ();
            return (TRUE);

        case WM_INITDIALOG :
            if (pszTitle && GetDlgItem(hDlg, 99))
            {
                char *sznTitle = new char [lstrlen(GetSetupTitle())+1];
                lstrcpy(sznTitle,GetSetupTitle());
                SetWindowText (GetDlgItem (hDlg, 99), sznTitle);
                delete sznTitle;
            }
            SetFocus (hDlg);
            return (TRUE);

        case WM_INITMENU :
            hSysMenu = GetSystemMenu (hDlg, FALSE);
            EnableMenuItem (hSysMenu, SC_CLOSE, MF_GRAYED);
            return (TRUE);
        }
    return (FALSE);
}
/*@Method
   This method destroys the abort dialog.  It is called internally only.
*/
void Printer::DestroyAbortWnd ()
{
    DestroyWindow (hwndAbortDlg);
    hwndAbortDlg = NULL;
}
char *Printer::GetPrintFile(char *szFileName)
{
	HANDLE hFile = EBUCreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD size,highsize,bytesread;
	char *ptr;
	if (INVALID_HANDLE_VALUE == hFile)
		return (char *)NULL;
	size = GetFileSize(hFile,&highsize);
	ptr = new char[size+1];
	ZeroMemory(ptr, size + 1);
	if (!EBUReadFile(hFile, ptr, size, &bytesread, NULL))
	{
		CloseHandle(hFile);
		return(NULL);
	}
	CloseHandle(hFile);
	return ptr;
}
boolean Printer::PrintFile(char *szFileName)
{
	char *ptr = GetPrintFile(szFileName);
	if(ptr)
	{
		boolean retc = PrintMultiLine(ptr);
		delete ptr;
		return retc;
	}
	return FALSE;
}
BOOL Printer::IsDefaultPrinter()
{
	LPBYTE pPrinterEnum = new BYTE[1024*10];
	DWORD cbNeeded;
	DWORD cReturned;
	if(EnumPrinters( PRINTER_ENUM_DEFAULT, NULL,1,pPrinterEnum,1024*10,&cbNeeded,&cReturned) == FALSE)
	{
		delete pPrinterEnum;
		return FALSE;
	}
	if(cReturned == 0 && (GetOS() & OS_NTMASK))
	{
	   if(EnumPrinters( PRINTER_ENUM_CONNECTIONS, NULL,1,pPrinterEnum,1024*10,&cbNeeded,&cReturned) == FALSE)
	   {
		   delete pPrinterEnum;
		   return 0;
	   }
	}

	delete pPrinterEnum;
	return (cReturned > 0) ? TRUE : FALSE;
}
 

extern "C"
{
/*@Function
   This function is a message pump for the print dialog routines independent
of the messenger message pump in order to process aborts.
*/
 _declspec(dllexport) BOOL WINAPI AbortProc (HDC hPr, int reserved)
{
    MSG msg;
	if(!g_pPrint)
		return TRUE;

    while (! g_pPrint->bAbort && PeekMessage (&msg, NULL, NULL, NULL, TRUE))
        if (! g_pPrint->hwndAbortDlg || ! IsDialogMessage (g_pPrint->hwndAbortDlg, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    return (! g_pPrint->bAbort);
}
/*@Function
   This function is a C encapsulation for the abortDlgProc.  It is a message
callback handler.
*/
_declspec(dllexport) int WINAPI AbortDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(!g_pPrint)
		return FALSE;
    return g_pPrint->abortDlgProc (hDlg, msg, wParam, lParam);
}
}


