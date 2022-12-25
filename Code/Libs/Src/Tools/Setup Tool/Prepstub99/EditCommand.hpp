#pragma once
#ifndef __EDITCOMMANDS_H
#define __EDITCOMMANDS_H

#include "windows.h"
#include "CListView.hpp"
#include "script.h"
#include "resource.h"
#include "textdoc.h"
#include "command.h"
#include "stdio.h"
#include "stateinfo.h"

#define BUILD_SETTINGS			0
#define	BUILD_SAVEAS			1
#define	BUILD_INJECT			2
#define BUILD_REPLICATE			3
#define BUILD_UPDATEFILELIST	4
#define BUILD_TRIAL				5
#define BUILD_UPDATE_AND_INJECT	6


extern KEYWORD Keywords[];
extern HINSTANCE g_hAppInst;
extern HWND g_hAppWnd;
extern BOOL FolderBrowse (char *szRootPath, char *szInstructions);
extern BOOL FileSaveAsDialog (HWND hWndParent, char *szFileName, UINT uFilter, int *nType);
extern void MakePathCompliant (char *szPath);





typedef struct tagCmdInfo
{
	char szToken[MAX_PATH];

} CMD_INFO;


enum BuildFlag {
	BF_WIN95=0,
	BF_WIN98,
	BF_NT40,
	BF_NT50,
	BF_DBCS,
	BF_OEM,
	BF_RTL,
	BF_JPN,
	BF_GER,
	BF_FRA,
	BF_SPA,
	BF_USA,
	BF_APP1,
	BF_APP2,
	BF_APP3,
	BF_IMEON,
	BF_IMEENABLE,
	BF_CABPRECOPY,
	NUM_FLAGS
};


void ClearStateInfo (STATEINFO *si);
void ClearGroupStateInfo (STATEINFO *si);
bool SetBuildFlagsFromTriState (DWORD *dwFlags, STATEINFO *si);
bool SetGroupFlagsFromTriState (__int64	*iGroup, STATEINFO *si);
BOOL CheckFlagsState (BuildFlag bf, DWORD dwFlags);
void InitializeGroupStateFromData (__int64 iGroup, STATEINFO *si);
void InitializeStateFromData (DWORD dwFlags, STATEINFO *si);
void SetListViewFromState (HWND hwndLV, STATEINFO *si);
void SetFlagNames (BuildFlag bf, STATEINFO *si);

BOOL CALLBACK EditCommandDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK BuildDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK VerificationDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK StringSettingsDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AdvancedInjectSettingsDialogProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);







bool SetCommandInfoInListView (HWND hwndLV, STATEINFO StateInfo, STATEINFO GroupStateInfo);

#endif