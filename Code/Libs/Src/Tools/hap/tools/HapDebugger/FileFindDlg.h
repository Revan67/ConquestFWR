//////////////////////////////////////////////////////////////////////////////
//  File:    FileFindDlg.h
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

#if !defined(AFX_FILEFINDDLG_H__B8BD2DFD_9237_42EC_A843_1C1144AF5B6B__INCLUDED_)
#define AFX_FILEFINDDLG_H__B8BD2DFD_9237_42EC_A843_1C1144AF5B6B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//structure to hold stored info about found files.
struct SFoundFileInfo
{
	const char* pPathFile;
	FILETIME ModifiedTime;
	DWORD dwFileSize;
	DWORD dwAttributes;
	int nLineNumber;
};


/////////////////////////////////////////////////////////////////////////////
// CFileFindDlg dialog

class CFileFindDlg : public CDialog
{
// Construction
public:
	CFileFindDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFileFindDlg)
	enum { IDD = IDD_FILEFIND_DLG };
	CString	m_FileType;
	CString	m_FindText;
	CString	m_FindFolder;
	BOOL	m_bMatchCase;
	BOOL	m_bMatchStart;
	BOOL	m_bMatchWholeWord;
	BOOL	m_bRegExp;
	//}}AFX_DATA
	BOOL m_bRecursive;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileFindDlg)
	public:
	virtual BOOL Create(CWnd* pParentWnd);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	HANDLE m_hFindThread;
	BOOL m_bCancelFind;

	static void __cdecl FileFindThreadFn(void* pParam);
	static void FindFiles(const char* FilePath, CFileFindDlg* pDlg);
	static void CheckFileForText(const char* FilePath, const char* SearchText, CFileFindDlg* pDlg);
	static void FindTextInFile(SFoundFileInfo* pFileInfo, const char* SearchText, vector<char>& FileBuffer, CFileFindDlg* pDlg);
	static void FindRegExpInFile(SFoundFileInfo* pFileInfo, const char* RegExpText, vector<char>& FileBuffer, CFileFindDlg* pDlg);

	static int CALLBACK BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData);

	void InsertFindText(const char* cszInsert, int nSelect=-1);
	void LoadFindHistory();

	// Generated message map functions
	//{{AFX_MSG(CFileFindDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnRegexBtn();
	afx_msg void OnFolderbrowseBtn();
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnDblclkResultsLst(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnFindpopAnychar();
	afx_msg void OnFindpopCharinset();
	afx_msg void OnFindpopCharnotinset();
	afx_msg void OnFindpopEndofline();
	afx_msg void OnFindpopEndword();
	afx_msg void OnFindpopOnemore();
	afx_msg void OnFindpopStartofline();
	afx_msg void OnFindpopStartword();
	afx_msg void OnFindpopTabchar();
	afx_msg void OnFindpopTagpart();
	afx_msg void OnFindpopZeromore();
	afx_msg void OnRegexpChk();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	afx_msg LRESULT OnFileFound(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFindComplete(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};


#endif // !defined(AFX_FILEFINDDLG_H__B8BD2DFD_9237_42EC_A843_1C1144AF5B6B__INCLUDED_)
