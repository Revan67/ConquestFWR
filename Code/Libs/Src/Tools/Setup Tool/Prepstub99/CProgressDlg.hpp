/**************************************************************
* CProgressDlg.hpp: Generic progress dialog class
*
* Chris N. Haddan
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/

#pragma once
#ifndef __CPROGRESSDLG_H
#define __CPROGRESSDLG_H

#include "windows.h"

class CProgressDialog
{
	public:
		CProgressDialog ();
		~CProgressDialog ();
		LRESULT CALLBACK ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		BOOL Create (HINSTANCE hInst, LPCSTR lpTemplate, HWND hwndParent);
		BOOL Create (const char *szLogFile);
		void WriteLogFileMessage (const char *szMessageType, const char *szMessage);
		void SetGranularity (DWORD dwGran);
		void SetRange (DWORD dwMin, DWORD dwMax);
		void Increment ();
		void ClearCancelRequest () { m_bCancelRequested = false; };
		bool IsCancelRequested () { return m_bCancelRequested; };
		void SetPos (DWORD dwPos);
		void SetCompleteAndWait();
		void SetCompleteAndWait(char *szTask, char *szStatus);
		void ForceFullProgressBar ();
		void SetStatusText (const char *pszText);
		void SetWindowText (char *pszText);
		void SetCurrentTask (char *pszTask);
		void SetComplete();
		void Close ();
		void Destroy(void);
		HWND GetHwnd() { return m_hWnd; };

	private:
		bool m_bCancelRequested;
		HWND m_hWnd;
		HWND m_hWndParent;
		DWORD m_dwCurrent;
		DWORD m_dwMin;
		DWORD m_dwMax;
		DWORD m_dwGranularity;
		char m_szLogFile[MAX_PATH];
		bool m_bCommandLineOutput;
		bool m_bLoggingOn; 
};

BOOL CALLBACK ProgressDialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif