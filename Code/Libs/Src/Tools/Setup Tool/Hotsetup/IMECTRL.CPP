//
// IMECtrl.cpp
//
//    Functions for controlling the IME.
//
// History:
//
//     Jul.23,1997 yutakah Created
//

#include <windows.h>
#include <imm.h>
#include "hotsetup.h"
#include "script.h"
#include "imectrl.h"

static HIMC (WINAPI *lpfnImmCreateContext) (void);
static HIMC (WINAPI *lpfnImmAssociateContext) (HWND, HIMC); 
static BOOL (WINAPI *lpfnImmDestroyContext) (HIMC);	
static HIMC (WINAPI *lpfnImmGetContext) (HWND);
static BOOL (WINAPI *lpfnImmSetOpenStatus) (HIMC, BOOL);
static BOOL (WINAPI *lpfnImmReleaseContext) (HWND, HIMC);
static BOOL (WINAPI *lpfnImmNotifyIME) (HIMC, DWORD, DWORD, DWORD);
static HWND (WINAPI *lpfnImmGetDefaultIMEWnd) (HWND);

static BOOL g_fIMEOK = FALSE;
static HINSTANCE g_hIMEDll = NULL;
static BOOL g_fInitialized = FALSE;

//****************************************************************************
// Procedure    changeIMEStatus
//
// Purpose      Change IME's status Enable(and ON/OFF) or Disable
//
// Parameters   HWND hWnd - target window handle
//              HWND hDlg - target dialog handle
//              UINT nEdit - number of edit control in the hDlg
//              WORD fIME - operation
//                          SCF_IME_ENABLE            - Enable & Turn OFF
//                          SCF_IME_ENABLE|SCF_IME_ON - Enable & Turn ON
//                          SCF_IME_DISABLE           - Disable
//              HIMC *hOldIMC - address of HIMC
//              HIMC *phIMC - address of HIMC[0]
//              * When disables the IME using SCF_IME_DISABLE, 
//                this function stores the previous HIMC.
//              * When enables the IME using SCF_IME_ENABLE, 
//                this function uses HIMC(s) handed via these variable.
//
// Returns      nothing.
//
// Comment      I recommend to use changeIMEStatus( hWnd, fIME ).
//              When using this function, we don't need to restore previous 
//              HIMC when destroy the edit control etc...
//
void IMEInitialize(void)
{
	if (g_fInitialized) return;

	if (FALSE == IsDBCS())
	{
		return;
	}

	g_fInitialized = TRUE;

	//
	//
	//Load operating system's IME support DLL 
	g_hIMEDll = LoadLibrary("IMM32.DLL");
	if (NULL == g_hIMEDll)
	{
		g_fIMEOK = FALSE;
		return;
	}

	//
	//Get pointers to all the functions we care about...
	//
	lpfnImmCreateContext    = (HIMC (WINAPI *) (void)) GetProcAddress(g_hIMEDll, "ImmCreateContext");
	lpfnImmAssociateContext = (HIMC (WINAPI *) (HWND, HIMC)) GetProcAddress(g_hIMEDll, "ImmAssociateContext");
	lpfnImmDestroyContext	= (BOOL (WINAPI *) (HIMC)) GetProcAddress(g_hIMEDll, "ImmDestroyContext");
	lpfnImmGetContext		= (HIMC (WINAPI *) (HWND)) GetProcAddress(g_hIMEDll, "ImmGetContext");
	lpfnImmSetOpenStatus	= (BOOL (WINAPI *) (HIMC, BOOL)) GetProcAddress(g_hIMEDll, "ImmSetOpenStatus");
	lpfnImmReleaseContext	= (BOOL (WINAPI *) (HWND, HIMC)) GetProcAddress(g_hIMEDll, "ImmReleaseContext");
	lpfnImmNotifyIME		= (BOOL (WINAPI *) (HIMC, DWORD, DWORD, DWORD)) GetProcAddress(g_hIMEDll, "ImmNotifyIME");
	lpfnImmGetDefaultIMEWnd = (HWND (WINAPI *) (HWND)) GetProcAddress(g_hIMEDll, "ImmGetDefaultIMEWnd");

	//
	//If any functions not found, disable IME support...
	//
	if (!lpfnImmCreateContext ||
		!lpfnImmAssociateContext ||
		!lpfnImmDestroyContext ||
		!lpfnImmGetContext ||
		!lpfnImmSetOpenStatus ||
		!lpfnImmReleaseContext ||
		!lpfnImmNotifyIME ||
		!lpfnImmGetDefaultIMEWnd)
	{
		IMEUninitialize();
	}

	g_fIMEOK = TRUE;
}

void IMEUninitialize(void)
{
	if (g_hIMEDll)
	{
		FreeLibrary(g_hIMEDll);
		g_hIMEDll = NULL;
	}

	g_fIMEOK = FALSE;
	g_fInitialized = FALSE;
}

void changeIMEStatus( HWND hWnd, WORD fIME )
{
	if (!g_fInitialized) IMEInitialize();
	
	if ( !hWnd || FALSE == g_fIMEOK ) return;
	
	if ( fIME & SCF_IME_ENABLE )
	{
		HIMC hIMC;
		
		// Aug.15,1997 07:41 by yutaka.
		// Force create a new context to initialize the IME.
		hIMC = (*lpfnImmCreateContext) ();

		hIMC = (*lpfnImmAssociateContext) ( hWnd, hIMC );
		if ( hIMC )
		{
			(*lpfnImmDestroyContext) ( hIMC );
		}
		
		hIMC = (*lpfnImmGetContext) ( hWnd );
		if ( !hIMC )
		{ // just for safe
			hIMC = (*lpfnImmCreateContext) ();
			(*lpfnImmAssociateContext) ( hWnd, hIMC );
			hIMC = (*lpfnImmGetContext) ( hWnd );
		}
		if ( hIMC )
		{
			(*lpfnImmSetOpenStatus) ( hIMC, 
				(fIME & SCF_IME_ON) ? TRUE/*Turn On*/ : FALSE );
			(*lpfnImmReleaseContext) ( hWnd, hIMC);
		}
	}
	else
	{ // disable
		HIMC hOldIMC = (*lpfnImmAssociateContext) ( hWnd, NULL );
		if ( hOldIMC )
		{
			(*lpfnImmDestroyContext) ( hOldIMC );
		}
	}
}

void changeIMEStatus( HWND hWnd, HIMC *hOldIMC, WORD fIME )
{
	if (!g_fInitialized) IMEInitialize();

	if ( !hWnd || FALSE == g_fIMEOK  ) return;
	
	if ( fIME & SCF_IME_ENABLE )
	{ // Enable
		// if already disabled, enable again.
		if ( *hOldIMC )
		{
			(*lpfnImmAssociateContext) ( hWnd, *hOldIMC );
			*hOldIMC = NULL;
		}
		
		HIMC hIMC;

	 // Aug.15,1997 07:41 by yutaka.
		// Force create a new context to initialize the IME.
		hIMC = (*lpfnImmCreateContext) ();
		hIMC = (*lpfnImmAssociateContext) ( hWnd, hIMC );
		if ( hIMC )
		{
			(*lpfnImmDestroyContext) ( hIMC );
		}
 
		hIMC = (*lpfnImmGetContext) ( hWnd );
		if ( !hIMC )
		{ // just for safe
			hIMC = (*lpfnImmCreateContext) ();
			(*lpfnImmAssociateContext) ( hWnd, hIMC );
			hIMC = (*lpfnImmGetContext) ( hWnd );
		}
		if ( hIMC )
		{
			(*lpfnImmSetOpenStatus) ( hIMC, 
				(fIME & SCF_IME_ON) ? TRUE/*Turn On*/ : FALSE );
			(*lpfnImmReleaseContext) ( hWnd, hIMC);
		}
	}
	else
	{ // Disable
		*hOldIMC = (*lpfnImmAssociateContext) ( hWnd, NULL );
	}
}

void changeIMEStatus( HWND hDlg, HIMC *phIMC, UINT nEdit, WORD fIME )
{
	if (!g_fInitialized) IMEInitialize();

	if ( !hDlg  || FALSE == g_fIMEOK ) return;
	
	UINT nProcessed = 0;
	char szBuf[128];
	HWND hWnd = GetWindow( hDlg, GW_CHILD ); // get first child window.
	
	fIME &= SCF_IME_MASK;
	
	for ( ; hWnd ; )
	{
		szBuf[0] = '\0';
		GetClassName( hWnd, szBuf, sizeof(szBuf) );
		if ( lstrcmpi( szBuf, "Edit" ) == 0 )
		{
			
			changeIMEStatus( hWnd, &phIMC[nProcessed], fIME );
			
			if ( ++nProcessed >= nEdit )
			{
				break;
			}
		}
		hWnd = GetNextWindow( hWnd, GW_HWNDNEXT );
	}
}

//****************************************************************************
// Procedure    clearIMEUndeterminedString
//
// Purpose      Clear IME's undetermined string
//
// Parameters   HWND hWnd - target window handle
//              HWND hDlg - target dialog handle
//              UINT nEdit - number of edit control in the hDlg
//              DWORD dwFlag - operation default=CPS_CANCEL
//                             <imm.h>
//                             #define CPS_COMPLETE 0x0001
//                             #define CPS_CONVERT  0x0002
//                             #define CPS_REVERT   0x0003
//                             #define CPS_CANCEL   0x0004
//
// Returns      nothing.
//
// Comment      Default operation is CANCEL. But we can use the other
//              operations described above.
//

void clearWndIMEUndeterminedString( HWND hWnd, DWORD dwFlag )
{
	HIMC hIMC;

	if (!g_fInitialized) IMEInitialize();

	if ( !hWnd || FALSE == g_fIMEOK) return;

	hIMC = (*lpfnImmGetContext) ( hWnd );
	if ( hIMC )
	{
		(*lpfnImmNotifyIME) ( hIMC, NI_COMPOSITIONSTR, dwFlag, 0 );
		(*lpfnImmReleaseContext) ( hWnd, hIMC);
	}
}

void clearDlgIMEUndeterminedString( HWND hDlg, UINT nEdit, DWORD dwFlag )
{
	UINT nProcessed = 0;
	char szBuf[128];

	if (!g_fInitialized) IMEInitialize();

	if (FALSE == g_fIMEOK) return;

	 HWND hWnd = GetWindow( hDlg, GW_CHILD ); // get first child window.
	
	for ( ; hWnd ; )
	{
		szBuf[0] = '\0';
		GetClassName( hWnd, szBuf, sizeof(szBuf) );
		if ( lstrcmpi( szBuf, "Edit" ) == 0 )
		{
			
			clearWndIMEUndeterminedString( hWnd, dwFlag );
			
			if ( ++nProcessed >= nEdit )
			{
				break;
			}
		}
		hWnd = GetNextWindow( hWnd, GW_HWNDNEXT );
	}
}

//****************************************************************************
// Procedure    hideIMEToolBar & showIMEToolBar
//
// Purpose      Hide & Show the IME toolbar
//
// Parameters   HWND hWnd - target window handle
//
// Returns      nothing.
//
// Comment      For the IME98's strange behavior.
//              After we call the MessageBox() etc, IME98 displays grayed
//              IME toolbar to the IME disabled hWnd. So we need to hide
//              them by this function.
//

void hideIMEToolBar( HWND hWnd )
{
//	HIMC hIMC;

	if (!g_fInitialized) IMEInitialize();

	if ( !hWnd || FALSE == g_fIMEOK) return;

// Hide always.
//
//	hIMC = (*lpfnImmGetContext) ( hWnd );
//	if ( hIMC ) // only if IME is disabled.
//	{
//		(*lpfnImmReleaseContext) ( hWnd, hIMC);
//	} else {
		SendMessage( (*lpfnImmGetDefaultIMEWnd)(hWnd), WM_IME_NOTIFY, 
			IMN_CLOSESTATUSWINDOW,
			0 );
//	}
}

void showIMEToolBar( HWND hWnd )
{
	HIMC hIMC;

	if (!g_fInitialized) IMEInitialize();

	if ( !hWnd || FALSE == g_fIMEOK) return;

	hIMC = (*lpfnImmGetContext) ( hWnd );
	if ( hIMC ) // only if IME is enabled.
	{
		SendMessage( (*lpfnImmGetDefaultIMEWnd)(hWnd), WM_IME_NOTIFY, 
			IMN_OPENSTATUSWINDOW,
			0 );
		(*lpfnImmReleaseContext) ( hWnd, hIMC);
	}
}
