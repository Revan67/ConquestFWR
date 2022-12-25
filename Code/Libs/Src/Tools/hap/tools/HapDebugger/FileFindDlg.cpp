//////////////////////////////////////////////////////////////////////////////
//  File:    FileFindDlg.cpp
//
//  Purpose: Dialog box which performs a file grep and displays the results
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001 by Humongous Entertainment
//  
//  This library is free software; you can redistribute it and/or modify it 
//  under the terms of the GNU Lesser General Public License as published by 
//  the Free Software Foundation; either version 2.1 of the License, or  any 
//  later version.
//  
//  This library is distributed in the hope that it will be useful, but 
//  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
//  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public 
//  License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public License 
//  along with this library (see the file license.txt); if not, write to the 
//  Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//////////////////////////////////////////////////////////////////////////////	

#include "stdafx.h"
#include "dbgremote.h"
#include "FileFindDlg.h"
#include "scintilla\scintilla.h"

#include <process.h> 

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileFindDlg dialog


#define UM_FILEFOUND	WM_USER+1122
#define UM_FINDCOMPLETE	WM_USER+1123




CFileFindDlg::CFileFindDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFileFindDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFileFindDlg)
	m_FileType = _T("");
	m_FindText = _T("");
	m_FindFolder = _T("");
	m_bMatchCase = FALSE;
	m_bMatchStart = FALSE;
	m_bMatchWholeWord = FALSE;
	m_bRegExp = FALSE;
	//}}AFX_DATA_INIT
	m_bRecursive = TRUE;


	m_hFindThread = NULL;
}


void CFileFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFileFindDlg)
	DDX_CBString(pDX, IDC_FILETYPE_CBO, m_FileType);
	DDX_CBString(pDX, IDC_FINDTXT_CBO, m_FindText);
	DDX_CBString(pDX, IDC_FOLDER_CBO, m_FindFolder);
	DDX_Check(pDX, IDC_MATCHCASE_CHK, m_bMatchCase);
	DDX_Check(pDX, IDC_MATCHSTART_CHK, m_bMatchStart);
	DDX_Check(pDX, IDC_MATCHWORD_CHK, m_bMatchWholeWord);
	DDX_Check(pDX, IDC_REGEXP_CHK, m_bRegExp);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFileFindDlg, CDialog)
	//{{AFX_MSG_MAP(CFileFindDlg)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_REGEX_BTN, OnRegexBtn)
	ON_BN_CLICKED(IDC_FOLDERBROWSE_BTN, OnFolderbrowseBtn)
	ON_WM_CLOSE()
	ON_NOTIFY(NM_DBLCLK, IDC_RESULTS_LST, OnDblclkResultsLst)
	ON_COMMAND(ID_FINDPOP_ANYCHAR, OnFindpopAnychar)
	ON_COMMAND(ID_FINDPOP_CHARINSET, OnFindpopCharinset)
	ON_COMMAND(ID_FINDPOP_CHARNOTINSET, OnFindpopCharnotinset)
	ON_COMMAND(ID_FINDPOP_ENDOFLINE, OnFindpopEndofline)
	ON_COMMAND(ID_FINDPOP_ENDWORD, OnFindpopEndword)
	ON_COMMAND(ID_FINDPOP_ONEMORE, OnFindpopOnemore)
	ON_COMMAND(ID_FINDPOP_STARTOFLINE, OnFindpopStartofline)
	ON_COMMAND(ID_FINDPOP_STARTWORD, OnFindpopStartword)
	ON_COMMAND(ID_FINDPOP_TABCHAR, OnFindpopTabchar)
	ON_COMMAND(ID_FINDPOP_TAGPART, OnFindpopTagpart)
	ON_COMMAND(ID_FINDPOP_ZEROMORE, OnFindpopZeromore)
	ON_BN_CLICKED(IDC_REGEXP_CHK, OnRegexpChk)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
	ON_MESSAGE(UM_FILEFOUND, OnFileFound)
	ON_MESSAGE(UM_FINDCOMPLETE, OnFindComplete)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileFindDlg message handlers

BOOL CFileFindDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_RESULTS_LST);

	pLst->SetExtendedStyle(pLst->GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	pLst->InsertColumn(0, "File", LVCFMT_LEFT, 40, -1);
	
	pLst->InsertColumn(1, "Line", LVCFMT_RIGHT, 40, -1);
	pLst->InsertColumn(2, "Text", LVCFMT_LEFT, 40, -1);

	pLst->InsertColumn(3, "Type", LVCFMT_LEFT, 30, 1);
	pLst->InsertColumn(4, "Folder", LVCFMT_LEFT, 50, 2);
	pLst->InsertColumn(5, "Modified", LVCFMT_RIGHT, 30, 3);
	pLst->InsertColumn(6, "Size", LVCFMT_RIGHT, 30, 4);
	pLst->InsertColumn(7, "Attr", LVCFMT_LEFT, 30, 4);


	pLst->SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(3, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(4, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(5, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(6, LVSCW_AUTOSIZE_USEHEADER);


	LoadFindHistory();

	CStatic* pStc = (CStatic*)GetDlgItem(IDC_STATIC_NUMRESULTS);
	pStc->SetWindowText("Search Results - 0");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CFileFindDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	CWnd* pWnd = GetDlgItem(IDC_RESULTS_LST);
	if (pWnd == NULL)
		return;

	CRect r;
	pWnd->GetWindowRect(&r);
	ScreenToClient(&r);

	int nBorder = r.left;
	r.right = cx - nBorder;
	r.bottom = cy - nBorder;
	pWnd->MoveWindow(&r);
}

void CFileFindDlg::OnOK() 
{
	UpdateData(TRUE);
	
	CUserCfg* pUsrCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	pUsrCfg->AddFileFindPath(m_FindFolder); 
	pUsrCfg->AddFileFindTypes(m_FileType);
	pUsrCfg->AddFileFindWhat(m_FindText);

	LoadFindHistory();

	UpdateData(FALSE);

	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_RESULTS_LST);
	pLst->DeleteAllItems();

	if (m_hFindThread)
	{
		m_bCancelFind = TRUE;
		if (::WaitForSingleObject(m_hFindThread, 250) == WAIT_TIMEOUT)
		{
			TerminateThread(m_hFindThread, -1);
		}
		m_hFindThread = NULL;
	}

	m_bCancelFind = FALSE;
	m_hFindThread = (HANDLE)_beginthread(CFileFindDlg::FileFindThreadFn, 0, this);

	CStatic* pStc = (CStatic*)GetDlgItem(IDC_STATIC_NUMRESULTS);
	pStc->SetWindowText("Searching...");
	//CDialog::OnOK();
}

void CFileFindDlg::OnCancel() 
{
	m_bCancelFind = TRUE;
	CDialog::OnCancel();
}

void CFileFindDlg::OnClose() 
{
	if (m_hFindThread)
	{
		m_bCancelFind = TRUE;
		if (::WaitForSingleObject(m_hFindThread, 250) == WAIT_TIMEOUT)
		{
			TerminateThread(m_hFindThread, -1);
		}
		m_hFindThread = NULL;
	}
	
	CDialog::OnClose();
}


void CFileFindDlg::OnRegexBtn() 
{
	CComboBox* pFindCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	DWORD dwSel = pFindCbo->GetEditSel();


    CWnd* pRegexBtn = GetDlgItem(IDC_REGEX_BTN);
	if (pRegexBtn)
	{
		CRect r;
		pRegexBtn->GetWindowRect(&r);

		CMenu menu;
		if (menu.LoadMenu(IDR_FINDPOPUP))
		{
			CMenu* pPopup = menu.GetSubMenu(0);
			ASSERT(pPopup != NULL);

			pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON,
			                       r.left, r.top, this);
		}
	}	
}


int CALLBACK CFileFindDlg::BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData) 
{
	TCHAR szDir[MAX_PATH];

	switch(uMsg) 
	{
		case BFFM_INITIALIZED: 
		{
			if ( GetCurrentDirectory(sizeof(szDir)/sizeof(TCHAR),szDir))
			{
				// WParam is TRUE since you are passing a path.
				// It would be FALSE if you were passing a pidl.
				CFileFindDlg* pDlg = (CFileFindDlg*)pData;
				::SendMessage(hwnd,BFFM_SETSELECTION,TRUE,(LPARAM)((const char*)pDlg->m_FindFolder));
			}
			break;
		}
		case BFFM_SELCHANGED: 
		{
			// Set the status window to the currently selected path.
			if (SHGetPathFromIDList((LPITEMIDLIST) lp ,szDir)) 
			{
				::SendMessage(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)szDir);
			}
			break;
		}
	
	default:
		break;
	}
	return 0;
}

#ifndef BIF_NEWDIALOGSTYLE 
#define BIF_NEWDIALOGSTYLE 0x0040   // Added 10-15-02 Bruce Dickey. 
#endif                              // Works on NT2K and later only (Version 5.0 of Shell32.dll)


void CFileFindDlg::OnFolderbrowseBtn() 
{
	UpdateData(TRUE);
	BROWSEINFO bi;
	TCHAR szDir[MAX_PATH];
	
	LPMALLOC pMalloc;

	if (SUCCEEDED(SHGetMalloc(&pMalloc))) 
	{
		ZeroMemory(&bi,sizeof(bi));
		bi.hwndOwner = m_hWnd;
		bi.pszDisplayName = 0;
		bi.pidlRoot = 0;
		bi.ulFlags = BIF_RETURNONLYFSDIRS |/* BIF_STATUSTEXT | */BIF_EDITBOX | BIF_NEWDIALOGSTYLE;
		bi.lpfn = CFileFindDlg::BrowseCallbackProc;
		bi.lParam = (LPARAM)this;

		ITEMIDLIST* pidl = SHBrowseForFolder(&bi);
		if (pidl) 
		{
			if (SHGetPathFromIDList(pidl,szDir)) 
			{
				m_FindFolder = szDir;
				UpdateData(FALSE);
			}

			pMalloc->Free(pidl); 
			pMalloc->Release();
		}
	}
}

LRESULT CFileFindDlg::OnFindComplete(WPARAM wParam, LPARAM lParam)
{
	CStatic* pStc = (CStatic*)GetDlgItem(IDC_STATIC_NUMRESULTS);

	CString resultString;

	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_RESULTS_LST);
	resultString.Format("Search Results - %d\n", pLst->GetItemCount());
	pStc->SetWindowText(resultString);
	return 1;
}

LRESULT CFileFindDlg::OnFileFound(WPARAM wParam, LPARAM lParam)
{
	CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_RESULTS_LST);
	
	const char* cszFileLine = reinterpret_cast<const char*>(wParam);
	
	CString CurLine;
	if (cszFileLine)
	{
		const char* pLineEnd = strchr(cszFileLine, '\n');
		if (pLineEnd)
		{
			int len = pLineEnd - cszFileLine;
			char* pBuff = CurLine.GetBufferSetLength(len+1);
			memcpy(pBuff, cszFileLine, len);
			pBuff[len] = '\0';
			CurLine.ReleaseBuffer();
		}
		else
		{
			CurLine = cszFileLine;
		}
		
		int nLen = CurLine.GetLength();
		int i = 0;
		while (i < nLen)
		{
			if (isprint(CurLine[i]) == 0)
				CurLine.SetAt(i, ' ');
			i++;
		}
	}
	SFoundFileInfo *pFfi = reinterpret_cast<SFoundFileInfo*>(lParam);

	SHFILEINFO fi;
	SHGetFileInfo(pFfi->pPathFile, 0, &fi, sizeof(fi),
                  SHGFI_ATTRIBUTES|SHGFI_DISPLAYNAME|SHGFI_TYPENAME);

	//file name (no path)
	int ItemInx = pLst->InsertItem(pLst->GetItemCount(), fi.szDisplayName);

	//line number
	if (pFfi->nLineNumber > 0)
	{
		char LineNumBuff[32];
		sprintf(LineNumBuff, "%d", pFfi->nLineNumber);
		pLst->SetItemText(ItemInx, 1, LineNumBuff);

	}

	//line of text
	pLst->SetItemText(ItemInx, 2, CurLine);
	
	//file type
	pLst->SetItemText(ItemInx, 3, fi.szTypeName);

	//file folder
	CString Folder = pFfi->pPathFile;

	int nPathEnd = Folder.ReverseFind('\\');
	if (nPathEnd > -1)
		Folder.SetAt(nPathEnd, '\0');
	pLst->SetItemText(ItemInx, 4, Folder);
	
	
	//file modified time
	SYSTEMTIME SysTime;
	if (FileTimeToSystemTime(&(pFfi->ModifiedTime), &SysTime) )
	{
		#define DATESTRLEN	64
		char DateStr[DATESTRLEN];
		if (GetDateFormat(LOCALE_SYSTEM_DEFAULT, DATE_SHORTDATE,
               &SysTime, NULL, DateStr, DATESTRLEN) > 0)
		{
			CString DateTime = DateStr;
			if (GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, 
			      &SysTime, NULL, DateStr, DATESTRLEN) > 0)
			{
				DateTime += " ";
				DateTime += DateStr;
				
				pLst->SetItemText(ItemInx, 5, DateTime);
			}
		}
	}

	char FileSize[32];
	if (pFfi->dwFileSize >= 1048576)
	{
		sprintf(FileSize, "%d MB", (pFfi->dwFileSize >> 20));
	}
	else if (pFfi->dwFileSize >= 1024)
	{
		sprintf(FileSize, "%d KB", (pFfi->dwFileSize >> 10));
	}
	else
	{
		sprintf(FileSize, "%d Bytes", pFfi->dwFileSize);
	}
	
	pLst->SetItemText(ItemInx, 6, FileSize);

	CString Attr;
	if (pFfi->dwAttributes & FILE_ATTRIBUTE_READONLY)
		Attr += "R";
	if (pFfi->dwAttributes & FILE_ATTRIBUTE_HIDDEN)
		Attr += "H";
	if (pFfi->dwAttributes & FILE_ATTRIBUTE_SYSTEM)
		Attr += "S";
	if (pFfi->dwAttributes & FILE_ATTRIBUTE_COMPRESSED)
		Attr += "C";
	//if (pFfi->dwAttributes & FILE_ATTRIBUTE_ARCHIVE)
	//	Attr += "A";

	pLst->SetItemText(ItemInx, 7, Attr);

	pLst->SetColumnWidth(0, LVSCW_AUTOSIZE);
	//pLst->SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	pLst->SetColumnWidth(2, LVSCW_AUTOSIZE);
	pLst->SetColumnWidth(3, LVSCW_AUTOSIZE);
	pLst->SetColumnWidth(4, LVSCW_AUTOSIZE);
	pLst->SetColumnWidth(5, LVSCW_AUTOSIZE);
	pLst->SetColumnWidth(6, LVSCW_AUTOSIZE);
	//pLst->SetColumnWidth(7, LVSCW_AUTOSIZE);

	return 1;
}


void __cdecl CFileFindDlg::FileFindThreadFn(void* pParam)
{
	CFileFindDlg* pDlg = (CFileFindDlg*)pParam;
	
	CString SearchFolder = pDlg->m_FindFolder;
	if ((SearchFolder.GetLength() > 0) && 
		(SearchFolder[SearchFolder.GetLength()-1] != '\\') )
		SearchFolder += '\\';

	FindFiles(SearchFolder, pDlg);

	//the find in files is complete
	pDlg->SendMessage(UM_FINDCOMPLETE, (WPARAM)0, (LPARAM)0);
}

BOOL CompareStringWildcard(const char* cszCmpStr, const char* cszWildStr)
{
	assert(cszCmpStr && cszWildStr);
	const char* pCurCmp = cszCmpStr;
	const char* pCurWild = cszWildStr;
	while ((*pCurCmp != '\0') && (*pCurWild != '\0'))
	{
		if (*pCurWild == '?')
		{
			//question mark matches any character, move on
			pCurCmp++;
			pCurWild++;
			continue;
		}
		if (*pCurWild == '*')
		{
			//move to the next non wildcard char
			while ((*pCurWild != '\0') && ((*pCurWild == '?') || (*pCurWild == '*')))
				pCurWild++;

			//skip past characters until a match is found
			while ((*pCurCmp != '\0') && (*pCurCmp != *pCurWild))
				pCurCmp++;

			//if we haven't found a match break
			if (pCurCmp == '\0')
				break;
		}

		//otherwise the characters must match
		if (*pCurCmp != *pCurWild)
			return FALSE;

		//advance to the next char
		pCurCmp++;
		pCurWild++;
	}

	//match if both cur pointers are pointing at the ends.
	return ((*pCurCmp == '\0') && (*pCurWild == '\0'));


}


void CFileFindDlg::FindFiles(const char* FilePath, CFileFindDlg* pDlg)
{
	if (strlen(FilePath) < 1)
		return;

	CString SearchFilter = FilePath;
	if (SearchFilter[SearchFilter.GetLength()-1] != '\\')
		SearchFilter += '\\';
	SearchFilter += "*.*";
	
	WIN32_FIND_DATA fd;
	HANDLE hFind = ::FindFirstFile(SearchFilter, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fd.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			{
				if ((pDlg->m_bRecursive) && (strcmp(fd.cFileName, ".") != 0) && 
					                       (strcmp(fd.cFileName, "..") != 0))
				{
					CString NextPath = FilePath;
					NextPath += fd.cFileName;
					NextPath += '\\';
					FindFiles(NextPath, pDlg);
				}
			}
			else
			{
				if (CompareStringWildcard(fd.cFileName, pDlg->m_FileType))
				{
					CString MatchFile = FilePath;
					MatchFile += fd.cFileName;
					CheckFileForText(MatchFile, pDlg->m_FindText, pDlg);
				}
			}
		}while(::FindNextFile(hFind, &fd) && (pDlg->m_bCancelFind == FALSE));

		::FindClose(hFind);
	}
}





void CFileFindDlg::CheckFileForText(const char* FilePath, const char* SearchText, CFileFindDlg* pDlg)
{
	HANDLE hFile = ::CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
						FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	DWORD dwSizeHigh = 0;
	DWORD dwFileSize = GetFileSize(hFile, &dwSizeHigh);
	if (dwFileSize <= 0)
	{
		::CloseHandle(hFile);
		return;
	}

	SFoundFileInfo ffi;
	memset(&ffi, 0, sizeof(ffi));
	ffi.pPathFile = FilePath;
	ffi.dwFileSize = dwFileSize;
	

	BY_HANDLE_FILE_INFORMATION hfi;
	if (GetFileInformationByHandle(hFile, &hfi) )
	{
		ffi.ModifiedTime = hfi.ftLastWriteTime;
		ffi.dwAttributes = hfi.dwFileAttributes;
	}
	

	static vector<char> FileBuffer;
	FileBuffer.resize(dwFileSize);

	DWORD dwBytesRead = 0;
	BOOL bReadOk = ::ReadFile(hFile, &(FileBuffer[0]), dwFileSize, &dwBytesRead, NULL);
	::CloseHandle(hFile);

	if (bReadOk && (dwBytesRead == dwFileSize))
	{
		if (pDlg->m_bRegExp)
			FindRegExpInFile(&ffi, SearchText, FileBuffer, pDlg);
		else
			FindTextInFile(&ffi, SearchText, FileBuffer, pDlg);
	}
}

void CFileFindDlg::FindTextInFile(SFoundFileInfo* pFileInfo, const char* SearchText, vector<char>& FileBuffer, CFileFindDlg* pDlg)
{
	if (strlen(SearchText) == 0)
		return;

	CString SearchString = SearchText;

	int nSearchLen = SearchString.GetLength();

	if (pDlg->m_bMatchCase == FALSE)
		SearchString.MakeUpper();

	int nCurLine = 1;
	int nInxCurLineStart = 0;

	BOOL bMatchFirst;
	char FileChar;
	int nStrCmp;

	for (unsigned int nTestChr = 0; nTestChr < FileBuffer.size(); nTestChr++)
	{
		FileChar = FileBuffer[nTestChr];
		if (pDlg->m_bMatchCase == FALSE)
			// SearchText[0] needed a toupper() around it when we were ignoring case,
			// otherwise, an upper case letter was always being compared to a lower letter
			// and never matching.
			bMatchFirst = (toupper(FileChar) == toupper(SearchText[0]));
		else
			bMatchFirst = (FileChar == SearchText[0]);

		if (bMatchFirst)
		{
			// these were reversed. Matching with case is strncmp, ignore case is strnicmp
			if (pDlg->m_bMatchCase == FALSE)
				nStrCmp = strnicmp(&FileBuffer[0] + nTestChr, SearchText, nSearchLen);				
			else
				nStrCmp = strncmp(&FileBuffer[0] + nTestChr, SearchText, nSearchLen);				

			if (nStrCmp == 0)
			{
				const char* pSrcLine = &FileBuffer[0] + nInxCurLineStart;
				

				pFileInfo->nLineNumber = nCurLine;
				
				//store the match
				pDlg->SendMessage(UM_FILEFOUND, (WPARAM)pSrcLine, (LPARAM)pFileInfo);
				
				//move to the beginning of the next line
				while ((nTestChr < FileBuffer.size()) && (FileChar != '\n'))
					FileChar = FileBuffer[++nTestChr];
			}
		}
		
		if (FileChar == '\n')
		{
			nCurLine++;
			nInxCurLineStart = nTestChr+1;
		}
	}
}

int GetLineNumber(int nCharPos, vector<char>& FileBuffer)
{
	int nLine = 1;
	while (nCharPos > 0)
	{
		if (FileBuffer[nCharPos--] == '\n')
			nLine++;
	}
	return nLine;
}




void  CFileFindDlg::FindRegExpInFile(SFoundFileInfo* pFileInfo, const char* RegExpText, vector<char>& FileBuffer, CFileFindDlg* pDlg)
{

}
	




void CFileFindDlg::OnDblclkResultsLst(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NMITEMACTIVATE* pNmA = (NMITEMACTIVATE*)pNMHDR;
	if (pNmA->iItem > -1)
	{
		CListCtrl* pLst = (CListCtrl*)GetDlgItem(IDC_RESULTS_LST);
		CString File = pLst->GetItemText(pNmA->iItem, 0);
		CString Path = pLst->GetItemText(pNmA->iItem, 4);
		CString szLine = pLst->GetItemText(pNmA->iItem, 1);

		if (Path.GetLength() > 1)
		{
			if (Path[Path.GetLength() - 1] != '\\')
				Path += "\\";
		}
		Path += File;
		int nLine = 1;
		if (szLine.GetLength() > 0)
			nLine = strtol(szLine, NULL, 10);

		CDbgRemoteApp* pApp = (CDbgRemoteApp*)AfxGetApp();
		
		pApp->DisplayFileLine(Path, nLine);
	}
	
	*pResult = 0;
}

BOOL CFileFindDlg::Create(CWnd* pParentWnd) 
{	
	return CDialog::Create(IDD, pParentWnd);
}



void CFileFindDlg::InsertFindText(const char* cszInsert, int nSelect)
{
	CComboBox* pFindCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	DWORD dwSel = pFindCbo->GetEditSel();

	UpdateData(TRUE);

	DWORD dwSelStart = LOWORD(dwSel);
	DWORD dwSelEnd   = HIWORD(dwSel);
	if (dwSelStart != dwSelEnd)
	{
		m_FindText.Delete(_MIN(dwSelStart, dwSelEnd), abs(dwSelStart - dwSelEnd) + 1);
		dwSelStart  = _MIN(dwSelStart, dwSelEnd);
	}

	m_FindText.Insert(dwSelStart, cszInsert);
	UpdateData(FALSE);
	int nNewSelect = dwSelStart + nSelect;

	if (nSelect < 0)
	{
		nNewSelect = dwSelStart + strlen(cszInsert);
	}
	pFindCbo->SetEditSel(nNewSelect, nNewSelect);
	
}

	/*	Special characters interpreted are 
		'.' for any character, 
		'\(' and '\)' to tag parts of the match, 
		'\1' .. '\9' to refer to tagged parts, 
		'\<' to match the start of a word, 
		'\>' to match the end of a word, 
		'\' as a quote character, 
		'[' and ']' to indicate a set of characters, 
		^ within a set to complement the set, 
		^ outside a set to match the start of line, 
		$ to match the end of line, 
		* to match 0 or more times, 
		+ to match 1 or more times 
	*/



void CFileFindDlg::OnFindpopAnychar() 
{
	InsertFindText(".");
}

void CFileFindDlg::OnFindpopCharinset() 
{
	InsertFindText("[]", 1);	
}

void CFileFindDlg::OnFindpopCharnotinset() 
{
	InsertFindText("[^]", 2);
}

void CFileFindDlg::OnFindpopEndofline() 
{
	InsertFindText("$");
}

void CFileFindDlg::OnFindpopEndword() 
{
	InsertFindText("\\>");
}

void CFileFindDlg::OnFindpopOnemore() 
{
	InsertFindText("+");
}

void CFileFindDlg::OnFindpopStartofline() 
{
	InsertFindText("^");
}

void CFileFindDlg::OnFindpopStartword() 
{
	InsertFindText("\\<");
}

void CFileFindDlg::OnFindpopTabchar() 
{
	InsertFindText("\\t");
}

void CFileFindDlg::OnFindpopTagpart() 
{
	InsertFindText("\\(\\)", 2);
}

void CFileFindDlg::OnFindpopZeromore() 
{
	InsertFindText("*");
}

void CFileFindDlg::OnRegexpChk() 
{
	UpdateData(TRUE);
	if (m_bRegExp)
	{
		GetDlgItem(IDC_MATCHWORD_CHK)->EnableWindow(FALSE);
		GetDlgItem(IDC_MATCHSTART_CHK)->EnableWindow(FALSE);
		GetDlgItem(IDC_MATCHCASE_CHK)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_MATCHWORD_CHK)->EnableWindow(TRUE);
		GetDlgItem(IDC_MATCHSTART_CHK)->EnableWindow(TRUE);
		GetDlgItem(IDC_MATCHCASE_CHK)->EnableWindow(TRUE);
	}
}


void CFileFindDlg::LoadFindHistory()
{
	LockWindowUpdate();
	CUserCfg* pUsrCfg = ((CDbgRemoteApp*)AfxGetApp())->pGetUserCfg();
	
	CComboBox* pCbo = (CComboBox*)GetDlgItem(IDC_FINDTXT_CBO);
	pCbo->ResetContent();
	const vector<string>* pvecStrings = pUsrCfg->GetFileFindWhat();
	assert(pvecStrings);
	for (unsigned int i = 0; i < pvecStrings->size(); i++)
	{
		assert(pvecStrings->at(i).size() > 0);
		pCbo->InsertString(0, pvecStrings->at(i).c_str());
		
		pCbo->SelectString(0, pvecStrings->at(i).c_str());
	}
	
	pCbo = (CComboBox*)GetDlgItem(IDC_FILETYPE_CBO);
	pCbo->ResetContent();
	pvecStrings = pUsrCfg->GetFileFindTypes();
	assert(pvecStrings);
	for (unsigned int i = 0; i < pvecStrings->size(); i++)
	{
		assert(pvecStrings->at(i).size() > 0);
		pCbo->InsertString(0, pvecStrings->at(i).c_str());

		pCbo->SelectString(0, pvecStrings->at(i).c_str());
	}
	
	pCbo = (CComboBox*)GetDlgItem(IDC_FOLDER_CBO);
	pCbo->ResetContent();
	pvecStrings = pUsrCfg->GetFileFindPaths();
	assert(pvecStrings);
	for (unsigned int i = 0; i < pvecStrings->size(); i++)
	{
		assert(pvecStrings->at(i).size() > 0);
		pCbo->InsertString(0, pvecStrings->at(i).c_str());

		pCbo->SelectString(0, pvecStrings->at(i).c_str());
	}
	UnlockWindowUpdate();
}

void CFileFindDlg::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CDialog::OnShowWindow(bShow, nStatus);
	
	UpdateData(FALSE);
	((CComboBox*)GetDlgItem(IDC_FOLDER_CBO))->SetCurSel(0);
	((CComboBox*)GetDlgItem(IDC_FILETYPE_CBO))->SetCurSel(0);
}
