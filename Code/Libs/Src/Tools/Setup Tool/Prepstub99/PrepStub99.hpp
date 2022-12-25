/**************************************************************************
* 
* PrepStub98.hpp
* 
* Created 3/23/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/

#pragma once
#ifndef __PREPSTUB98_H
#define __PREPSTUB98_H
#include <windows.h>

#define APPLICATION_CLASS	"aas::PrepStub98Class"
#define APPLICATION_NAME	"PrepStub99"

#define PREPSTUB_PICKLIST_KEY "Software\\Microsoft\\Prepstub99\\Picklist\\"
#define MAX_PICKLIST_ITEMS 10


#define	CABFILE_RESOURCE_TYPE	"CabFile"
#define CABFILE_RESOURCE_NAME	"IDR_CABFILE"


//**************************************************************************
//* 
//* Global variables
//* 
//**************************************************************************

HCURSOR		g_hAppCursor = NULL;
HINSTANCE	g_hAppInst;
HWND		g_hAppWnd;
bool		g_bExiting = false;

// expected EbuPrepStub globals
HINSTANCE	g_hInst;
HWND		g_hwnd;
char		g_szAppTitle[256];
BOOL		g_bCommandLine = FALSE;
char		g_szCurrentPath[_MAX_PATH];
char		g_szLogFile[_MAX_PATH] = "PrepStub99.out";



//**************************************************************************
//* 
//* Function Prototypes
//* 
//**************************************************************************
extern void CenterWindowOnMonitor (HWND hWndParent, HWND hWnd);
LRESULT CALLBACK WndProc(HWND , UINT , WPARAM , LPARAM );
int  WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int);
BOOL CALLBACK AboutDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
ATOM InitApplication (HINSTANCE);
void EnableMenuOptions (DWORD dwState);
void SetViewMenuState (void);
void UpdateStatusBar ();
void MakePathCompliant (char *szPath);
void FixupConsoleOutput (void);


#endif