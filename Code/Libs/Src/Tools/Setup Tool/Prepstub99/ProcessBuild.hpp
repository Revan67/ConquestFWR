/**************************************************************************
* 
* ProcessBuild.hpp
* 
* Created 11/17/98 by Chris N. Haddan
* 
* (C) 1998 Microsoft Corporation
* 
***************************************************************************/
#pragma once
#ifndef __PROCESSBUILD_H
#define __PROCESSBUILD_H
#include <windows.h>

#define BASE_PICKLIST_ITEM 20000
#define	MO_DOC_OPEN		1
#define	MO_DOC_CLOSED	2

#define CD_HANDLE_UI	FALSE
#define CD_IGNORE_UI	TRUE

int  ProcessCommandLine (char *szCommandLine);
bool ProcessBuildCommand (HWND hWnd, DWORD dwBuildType);
bool ShellExecute (char *szProgram, char *szParams, int nShow);
DWORD ShellExecuteAndWait (char *szProgram, char *szParams, int nShow);
bool LoadScript(char *szScriptName);
#endif