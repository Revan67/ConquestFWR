/* file gauge.c */
#include "windows.h"
#include "string.h"
#include "gauge.h"
#include "resource.h"
#include "hotsetup.h"

using namespace NGLOBALS;

HWND  hwndProgressGizmo = NULL;
static INT     iCnt = 0;
static WNDPROC fpxProDlg;
static DWORD   rgbFG;
static DWORD   rgbBG;

extern HANDLE  hInst;
#define _MAX_STRING 200

#ifndef COLOR_3DFACE    // Windows 95 color definition
#define COLOR_3DFACE COLOR_BTNFACE
#endif

BOOL fUserQuit     = FALSE;
static BOOL fInsideGizmosQuitCode = FALSE;

#ifndef COLOR_HIGHLIGHT
  #define COLOR_HIGHLIGHT      (COLOR_APPWORKSPACE + 1)
  #define COLOR_HIGHLIGHTTEXT  (COLOR_APPWORKSPACE + 2)
#endif

#define COLORBG  rgbBG
#define COLORFG  rgbFG


/*
**   ProInit(hPrev, hInst)
**
**   Description:
**       This is called when the application is first loaded into
**       memory.  It performs all initialization.
**   Arguments:
**       hPrev  instance handle of previous instance
**       hInst  instance handle of current instance
**   Returns:
**       TRUE if successful, FALSE if not
***************************************************************************/
BOOL APIENTRY ProInit(HANDLE hPrev, HANDLE hInst)
{
	WNDCLASS rClass;


	if (!hPrev)
		{
		rClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		rClass.hIcon         = NULL;
		rClass.lpszMenuName  = NULL;
		rClass.lpszClassName = PRO_CLASS;
        rClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        rClass.hInstance     = GetResourceInst();
		rClass.style         = CS_HREDRAW | CS_VREDRAW;
		rClass.lpfnWndProc   = ProBarProc;
		rClass.cbClsExtra    = 0;
		rClass.cbWndExtra    = 2 * sizeof(WORD);

		if (!RegisterClass(&rClass))
			{
			return(FALSE);
			}
		}

		rgbBG = RGB(  0,   0, 255);
		rgbFG = RGB(255, 255, 255);

	return(TRUE);
}


/***************************************************************************/
VOID APIENTRY ProClear(HWND hdlg)
{

	if (!hdlg)
		hdlg = hwndProgressGizmo;

	SetDlgItemText(hdlg, ID_STATUS1, "");
	SetDlgItemText(hdlg, ID_STATUS2, "");
}


/*
**   ControlInit(hPrev, hInst)
**
**   Description:
**       This is called when the application is first loaded into
**       memory.  It performs all initialization.
**   Arguments:
**       hPrev  instance handle of previous instance
**       hInst  instance handle of current instance
**   Returns:
**       TRUE if successful, FALSE if not
***************************************************************************/


/*
**   ProDlgProc(hWnd, wMessage, wParam, lParam)
**
**   Description:
**       The window proc for the Progress dialog box
**   Arguments:
**       hWnd      window handle for the dialog
**       wMessage  message number
**       wParam    message-dependent
**       lParam    message-dependent
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
BOOL APIENTRY ProDlgProc(HWND hdlg, WORD wMessage, WPARAM wParam, LONG lParam)
{
    static char szProCancelMsg[_MAX_STRING];



	switch (wMessage)
		{
    case WM_INITDIALOG:
		{

		ProClear(hdlg);
		/* BLOCK */ /* centered on the screen - we really want this to be
					   a WS_CHILD instead of WS_POPUP so we can do something
					   intelligent inside the frame window */

		return(TRUE);
		}

	case WM_ACTIVATE:
		{
			if(LOWORD(wParam) == WA_INACTIVE)
			{
				if((HWND)lParam == GetParent(hdlg))
				{
					SetForegroundWindow(hdlg);
				}
			}
		}
		break;
    case WM_CLOSE:
        PostMessage(
            hdlg,
            WM_COMMAND,
            MAKELONG(IDCANCEL, BN_CLICKED),
            0L
            );
        return(TRUE);


	case WM_COMMAND:
		if (!fInsideGizmosQuitCode
                && LOWORD(wParam) == IDCANCEL)
            {

            fInsideGizmosQuitCode = TRUE;
            char szProCancelMsg[_MAX_STRING];
			EBULoadString(GetResourceInst(),STR_CANCELCOPY,szProCancelMsg,_MAX_STRING);

            if ( MessageBox(
                     hdlg,
                     (LPSTR)szProCancelMsg,
                     (LPSTR)GetAppTitle(),
                     MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2
                     ) == IDYES ) {

                fUserQuit = TRUE;
                fInsideGizmosQuitCode = FALSE;     
                return(TRUE);
			}

            fInsideGizmosQuitCode = FALSE;
            SendMessage(GetWndParent(), WM_NCACTIVATE, 1, 0L);
            }
        break;
		}

	return(0);
}


/*
**   ProBarProc(hWnd, wMessage, wParam, lParam)
**
**   Description:
**       The window proc for the Progress Bar chart
**   Arguments:
**       hWnd      window handle for the dialog
**       wMessage  message number
**       wParam    message-dependent
**       lParam    message-dependent
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
LONG APIENTRY ProBarProc(HWND hWnd, UINT wMessage, WPARAM wParam, LONG lParam)
{
	PAINTSTRUCT rPS;
	RECT        rc1, rc2;
    INT         dx, dy, x;
    WORD        iRange, iPos;
	char        rgch[30];
	INT         iHeight = 0, iWidth = 0;
	HFONT        hfntSav = NULL;
	static HFONT hfntBar = NULL;


	switch (wMessage)
		{
    case WM_CREATE:
		SetWindowWord(hWnd, BAR_RANGE, 10);
		SetWindowWord(hWnd, BAR_POS,    0);
		return(0L);

	case BAR_SETRANGE:
	case BAR_SETPOS:
        SetWindowWord(hWnd, wMessage - WM_USER, (WORD)wParam);
		InvalidateRect(hWnd, NULL, FALSE);
		UpdateWindow(hWnd);
		return(0L);

	case BAR_DELTAPOS:
		iRange = (WORD)GetWindowWord(hWnd, BAR_RANGE);
		iPos   = (WORD)GetWindowWord(hWnd, BAR_POS);

		if (iRange <= 0)
			iRange = 1;

		if (iPos > iRange)
			iPos = iRange;

		if (iPos + wParam > iRange)
			wParam = iRange - iPos;

		SetWindowWord(hWnd, BAR_POS, (WORD)(iPos + wParam));
        if ((iPos * 100 / iRange) < ((iPos + (WORD)wParam) * 100 / iRange))
			{
			InvalidateRect(hWnd, NULL, FALSE);
			UpdateWindow(hWnd);
			}
		return(0L);

	case WM_SETFONT:
        hfntBar = (HFONT)wParam;
		if (!lParam)
			return(0L);
		InvalidateRect(hWnd, NULL, TRUE);

	case WM_PAINT:
		BeginPaint(hWnd, &rPS);
		if (hfntBar)
			hfntSav = (HFONT)SelectObject(rPS.hdc, hfntBar);
		GetClientRect(hWnd, &rc1);
		FrameRect(rPS.hdc, &rc1, (HBRUSH)GetStockObject(BLACK_BRUSH));
		InflateRect(&rc1, -1, -1);
		rc2 = rc1;
        iRange = GetWindowWord(hWnd, BAR_RANGE);
        iPos   = GetWindowWord(hWnd, BAR_POS);

		if (iRange <= 0)
			iRange = 1;

		if (iPos > iRange)
			iPos = iRange;

		dx = rc1.right;
		dy = rc1.bottom;
		x  = (WORD)((DWORD)iPos * dx / iRange) + 1;

		if (iPos < iRange)
			iPos++;
		SIZE sz;
		wsprintf(rgch, "%3d%%", (WORD)((DWORD)iPos * 100 / iRange));
		GetTextExtentPoint32(rPS.hdc, rgch, strlen(rgch), &sz);

		rc1.right = x;
		rc2.left  = x;

		SetBkColor(rPS.hdc, COLORBG);
		SetTextColor(rPS.hdc, COLORFG);
		ExtTextOut(rPS.hdc, (dx-sz.cx)/2, (dy-sz.cy)/2,
				ETO_OPAQUE | ETO_CLIPPED, &rc1, rgch, strlen(rgch), NULL);

		SetBkColor(rPS.hdc, COLORFG);
		SetTextColor(rPS.hdc, COLORBG);
		ExtTextOut(rPS.hdc, (dx-sz.cx)/2, (dy-sz.cy)/2,
				ETO_OPAQUE | ETO_CLIPPED, &rc2, rgch, strlen(rgch), NULL);

		if (hfntSav)
			SelectObject(rPS.hdc, hfntSav);
		EndPaint(hWnd, (LPPAINTSTRUCT)&rPS);
		return(0L);
		}

	return(DefWindowProc(hWnd, wMessage, wParam, lParam));
}


/*
**   ProOpen ()
**
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
HWND APIENTRY ProOpen(HWND hwnd)
{


    //
    // Maintain the number of times the progress guage is opened
    //

    iCnt++;

    //
    // Check if progress guage still present
    //

	if (!hwndProgressGizmo)
        {

        //
        // Create the Progress dialog
        //

        hwndProgressGizmo = CreateDialog(GetResourceInst(), MAKEINTRESOURCE(PROGRESS), hwnd, (DLGPROC)ProDlgProc);

        // Show the guage and paint it
        //

		ShowWindow(hwndProgressGizmo, SW_NORMAL);
		InvalidateRect(hwndProgressGizmo, NULL, TRUE);
        UpdateWindow(hwndProgressGizmo);
		}

	return(hwndProgressGizmo);
}


/*
**   ProClose(GetWndParent())
**
**   Returns:
**       0 if processed, nonzero if ignored
***************************************************************************/
BOOL APIENTRY ProClose(HWND hWnd)
{

	iCnt--;
	if (hwndProgressGizmo && iCnt == 0)
        {
        EnableWindow(hWnd, TRUE);
		DestroyWindow(hwndProgressGizmo);
		FreeProcInstance(fpxProDlg);
		hwndProgressGizmo = NULL;
		}

	return(TRUE);
}


/***************************************************************************/
BOOL APIENTRY ProSetText(INT i, char *sz)
{

	if (hwndProgressGizmo)
        {
        SetDlgItemText(hwndProgressGizmo, i, sz);
        return(TRUE);
		}

	return(FALSE);
}


/***************************************************************************/
BOOL APIENTRY ProSetCaption(char *szCaption)
{
	if (hwndProgressGizmo)
		{
		SetWindowText(hwndProgressGizmo, szCaption);
		return(TRUE);
		}

	return(FALSE);
}


/***************************************************************************/
BOOL APIENTRY ProSetBarRange(INT i)
{

	if (hwndProgressGizmo)
		{
		SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_SETRANGE, i, 0L);
		return(TRUE);
		}

	return(FALSE);
}


/***************************************************************************/
BOOL APIENTRY ProSetBarPos(INT i)
{

	if (hwndProgressGizmo)
		{
		SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_SETPOS, i, 0L);
		return(TRUE);
		}

	return(FALSE);
}


/***************************************************************************/
BOOL APIENTRY ProDeltaPos(INT i)
{

	if (hwndProgressGizmo)
		{
		SendDlgItemMessage(hwndProgressGizmo, ID_BAR, BAR_DELTAPOS, i, 0L);
		return(TRUE);
		}

	return(FALSE);
}

