/**************************************************************
* CProgressDialog.hpp: Generic progress dialog class
*
* Chris N. Haddan
*
* (C) 1998 Microsoft Corporation
*
***************************************************************/
#include "CProgressDlg.hpp"
#include "resource.h"
#include "stdio.h"
#include "util.h"
#include "commctrl.h"
#include "windowsx.h"

extern HINSTANCE g_hAppInst;


void CProgressDialog::WriteLogFileMessage (const char *szMessageType, const char *szMessage)
{
	FILE *fp;
	fp = fopen (m_szLogFile, "a+");
	
	fstprintf (fp, STR_CON_PROGRESS, szMessageType, szMessage);	
	stprintf (STR_CON_PROGRESS, szMessageType, szMessage);	
	fclose (fp);
}


void CProgressDialog::SetStatusText (const char *pszText)
{
	if (m_bCommandLineOutput)
		WriteLogFileMessage ("Status", pszText);
	else
		SetDlgItemText (m_hWnd, IDC_STATUS, pszText);	
}


void CProgressDialog::SetPos (DWORD dwPos)
{
	if (m_bCommandLineOutput)
		return;

	m_dwCurrent = dwPos;
	SendMessage (GetDlgItem (m_hWnd, IDC_PROGRESS), PBM_SETPOS, dwPos, 0);
}


void CProgressDialog::SetComplete()
{
	if (m_bCommandLineOutput)
		return;

	SetDlgItemText (m_hWnd, IDCANCEL, "Complete");
}


void CProgressDialog::ForceFullProgressBar (void)
{
	if (m_bCommandLineOutput)
		return;

	m_dwCurrent = m_dwMax;
	Increment();
}


void CProgressDialog::SetCompleteAndWait()
{
	if (m_bCommandLineOutput)
		return;

	SetDlgItemText (m_hWnd, IDCANCEL, "Complete");
	ForceFullProgressBar ();
	MessageBeep (MB_OK);

	while (!IsCancelRequested()) 
	{
		EbuYield();
	}

	ClearCancelRequest();
	Close ();
	Destroy();
}


void CProgressDialog::SetCompleteAndWait(char *szTask, char *szStatus)
{
	if (m_bCommandLineOutput)
		return;

	SetCurrentTask (szTask);
	SetStatusText(szStatus);

	SetDlgItemText (m_hWnd, IDCANCEL, "Complete");
	ForceFullProgressBar ();
	MessageBeep (MB_OK);

	while (!IsCancelRequested()) 
	{
		EbuYield();
	}

	ClearCancelRequest();
	Close ();
	Destroy();
}


void CProgressDialog::SetWindowText (char *pszText)
{
	if (m_bCommandLineOutput)
		return;

	::SetWindowText (m_hWnd, pszText);
}


void CProgressDialog::Close ()
{
	if (m_bCommandLineOutput)
		return;

	EndDialog (m_hWnd, 0);
	EnableWindow (m_hWndParent, TRUE);
}


void CProgressDialog::Increment()
{
	if (m_bCommandLineOutput)
		return;

	if (m_dwCurrent + m_dwGranularity < m_dwMax )
		m_dwCurrent += m_dwGranularity;

	SendMessage (GetDlgItem (m_hWnd, IDC_PROGRESS), PBM_SETPOS, m_dwCurrent, 0);
}


void CProgressDialog::SetGranularity (DWORD dwGran)
{
	if (m_bCommandLineOutput)
		return;

	m_dwGranularity = dwGran;
}


void CProgressDialog::SetRange (DWORD dwMin, DWORD dwMax)
{
	if (m_bCommandLineOutput)
		return;

	m_dwMin = dwMin; 
	m_dwMax = dwMax;
	SendMessage (GetDlgItem (m_hWnd, IDC_PROGRESS), PBM_SETRANGE32, m_dwMin, m_dwMax);
}


BOOL CProgressDialog::Create (HINSTANCE hInst, LPCSTR lpTemplate, HWND hwndParent)
{
	m_bLoggingOn = false;
	m_bCommandLineOutput = false;
	m_bCancelRequested = false;
	m_dwCurrent = 0;

	m_hWndParent = hwndParent;
	m_hWnd = CreateDialog (hInst, lpTemplate, hwndParent, (DLGPROC)ProgressDialogProc);

	SetWindowLong (m_hWnd, GWL_USERDATA, (long)this);

	if (m_hWnd)
	{
		ShowWindow (m_hWnd, SW_SHOW);
		EnableWindow (hwndParent, FALSE);
		//EnableWindow (GetDlgItem (m_hWnd, IDC_TASKLIST), FALSE);
	}

	return ((m_hWnd == NULL)?FALSE:TRUE);
}

BOOL CProgressDialog::Create (const char *szLogFile)
{
	BOOL bExist = DoesFileExist (szLogFile);

	if (bExist)
		lstrcpy (m_szLogFile, szLogFile);
	else
		lstrcpy (m_szLogFile, "");

	m_bCommandLineOutput = true;
	m_bCancelRequested = false;
	m_dwCurrent = 0;

	m_hWndParent = NULL;
	m_hWnd = NULL; 
	m_bLoggingOn = (bExist?true:false);

	return (bExist);
}


CProgressDialog::CProgressDialog ()
{
	m_hWnd = NULL;
	m_hWndParent = NULL;
	m_bCancelRequested = false;
	m_dwCurrent = 0;
	m_dwMin = 0;
	m_dwMax = 0;
	m_dwGranularity = 0;
	m_bCommandLineOutput = false;
	m_bLoggingOn = false;
}


CProgressDialog::~CProgressDialog ()
{
	if (!m_bCommandLineOutput)
	{
		if (IsWindow (m_hWnd))
		{	
			EndDialog (m_hWnd, 0);
		}
	}

	m_bCommandLineOutput = true;
	m_hWnd = NULL;
	m_hWndParent = NULL;
	m_bCancelRequested = false;
	m_dwCurrent = 0;
	m_dwMin = 0;
	m_dwMax = 0;
	m_dwGranularity = 0;
	m_bLoggingOn = false;
	lstrcpy (m_szLogFile, "");
}


void CProgressDialog::Destroy(void)
{
	delete this;
}


void CProgressDialog::SetCurrentTask (char *pszTask)
{
	if (m_bLoggingOn && m_bCommandLineOutput)
		WriteLogFileMessage ("Task", pszTask);
	else
	{
		ListBox_SetTopIndex (GetDlgItem (m_hWnd, IDC_TASKLIST),ListBox_InsertString (GetDlgItem (m_hWnd, IDC_TASKLIST), ListBox_GetCount (GetDlgItem (m_hWnd, IDC_TASKLIST)), pszTask));
		EbuYield();
	}
}


BOOL CALLBACK ProgressDialogProc (HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CProgressDialog *pProgressDialog = (CProgressDialog *)GetWindowLong (hwndDlg, GWL_USERDATA);

	// we dont' have a instianated object yet, so just let windows handle these messages...
	if (pProgressDialog==NULL)
	{
		return 0;
	}
	
	// dispatch messages to the object's ProcessMessage handler
	return (pProgressDialog->ProcessMessages (hwndDlg, uMsg, wParam, lParam));
}


LRESULT CALLBACK CProgressDialog::ProcessMessages (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_COMMAND:
			switch (wParam) 
			{
				case IDCANCEL:
					m_bCancelRequested = true;
					break;
			}
			return 0;
	}
	return 0;
}