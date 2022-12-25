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
#pragma once


// CBrowseSelDlg dialog

class CBrowseSelDlg : public CDialog
{
	DECLARE_DYNAMIC(CBrowseSelDlg)

public:
	CBrowseSelDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBrowseSelDlg();

	void AddBrowseItem(const char* cszFile, int nLine);
	int SelectBrowseItem();

// Dialog Data
	enum { IDD = IDD_BROWSESEL_DLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	struct SBrowseItem
	{
		SBrowseItem(const string& inFile, int inLine)
		{
			file = inFile;
			line = inLine;
		}
		
		string file;
		int line;
	};

	vector<SBrowseItem> m_browseItems;
	int m_SelectedItem;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnNMDblclkBrowseCltl(NMHDR *pNMHDR, LRESULT *pResult);
};
