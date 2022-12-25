//////////////////////////////////////////////////////////////////////////////
//  File:    FilePropDlg.cpp
//
//  Purpose: A dialog box that displays file properties
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

// FilePropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dbgremote.h"
#include "FilePropDlg.h"
#include "SourceControlInterface.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFilePropDlg dialog


CFilePropDlg::CFilePropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFilePropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFilePropDlg)
	m_Filename = _T("");
	m_FileStatus = _T("");
	m_FilePath = _T("");
	m_ModifiedDate = _T("");
	//}}AFX_DATA_INIT
}


void CFilePropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFilePropDlg)
	DDX_Text(pDX, IDC_FILENAME_EDT, m_Filename);
	DDX_Text(pDX, IDC_FILESTATUS_EDT, m_FileStatus);
	DDX_Text(pDX, IDC_FULLPATH_EDT, m_FilePath);
	DDX_Text(pDX, IDC_MODIFIED_EDT, m_ModifiedDate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFilePropDlg, CDialog)
	//{{AFX_MSG_MAP(CFilePropDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFilePropDlg message handlers

BOOL CFilePropDlg::Create(CWnd* pParentWnd) 
{	
	return CDialog::Create(IDD, pParentWnd);
}

void CFilePropDlg::ShowFileProperties(const char* filename)
{
	WIN32_FILE_ATTRIBUTE_DATA fad;
	BOOL bAttrib = GetFileAttributesEx(filename, GetFileExInfoStandard, &fad);
	
	string FilePath = filename;
	int nPathEnd = FilePath.find_last_of ("\\");
	if (nPathEnd > -1)
	{
		m_Filename = FilePath.c_str();
		m_Filename = m_Filename.Right(m_Filename.GetLength() - nPathEnd);

		m_FilePath = FilePath.c_str();
		m_FilePath = m_FilePath.Left(nPathEnd);
	}
	else
	{
		m_Filename = FilePath.c_str();
		m_FilePath = "";
	}

	m_FileStatus = "";
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
	{
		m_FileStatus += "read only ";
	}
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
	{
		m_FileStatus += "compressed ";
	}
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
	{
		m_FileStatus += "archive ";
	}
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
	{
		m_FileStatus += "encrypted ";
	}
	if (fad.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
	{
		m_FileStatus += "hidden";
	}

	SScFileStatus stat = ((CDbgRemoteApp*)AfxGetApp())->GetSci()->GetFileStatus(FilePath.c_str()); 

	if (stat.bControlled)
	{
		m_FileStatus += "scc: ";
		if (stat.bOpenForEdit)
			m_FileStatus += "open for edit ";
		else
			m_FileStatus += "in ";

		if (stat.bNewerInSc)
			m_FileStatus += "newer avail ";		
		if (stat.bOpenByOther)
			m_FileStatus += "open by other ";
	}
	
	m_ModifiedDate = "";

	FILETIME locFt;
	FileTimeToLocalFileTime(&fad.ftLastWriteTime, &locFt);

	SYSTEMTIME st;
	FileTimeToSystemTime(&locFt, &st);

	CTime Time(st);
	m_ModifiedDate = Time.Format("%I:%M %p, %m/%d/%Y");

	DoModal();
}
