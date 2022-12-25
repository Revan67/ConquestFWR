//////////////////////////////////////////////////////////////////////////////
//  File:    ProjectViewBar.h
//
//  Purpose: This is a dockable window that displays the current project (if any)
//  it also displays the current modules loaded by the python debugger.
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

#if !defined(AFX_PROJECTVIEWBAR_H__05D6261E_3F7A_453D_938A_174DD5752E84__INCLUDED_)
#define AFX_PROJECTVIEWBAR_H__05D6261E_3F7A_453D_938A_174DD5752E84__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "sizecbar.h"
#include "scbarg.h"
#include "SourceControlInterface.h"
#include <set>


class CPdpManager;

/////////////////////////////////////////////////////////////////////////////
// CProjectViewBar window

class CProjectViewBar : public CSizingControlBarG
{
// Construction
public:
	CProjectViewBar();
	virtual ~CProjectViewBar();


	void UpdateModules(const char* cszModuleList);
	void UpdateStyles();
	CString GetFilePathName(const char* cszFilename)const;

	void UpdateProjectDisplay(CPdpManager* pProject);
	void UpdateProjectSettings(CPdpManager* pProject);
	void UpdateFileStatus(const char* cszFile, CPdpManager* pProject);
	
protected:
	CTreeCtrl m_ModuleTree;
	CTreeCtrl m_ProjectTree;
	CTreeCtrl m_ClassTree;

	CImageList m_FileIcons;

	CImageList* m_pDragImgList;
	bool m_bDragging;
	int  m_nDragInx;

	CTabCtrl m_Tab;
	HBRUSH m_hBkBrush;
	CFont m_ProjectFont;
	CFont m_ModulesFont;
	CFont m_TabFont;

	CStringArray m_Filenames;

	struct SProjectItem
	{
		string strFilename;
		SScFileStatus status;
		bool   bBootFile;
		string strGroup;
		int nInx;

		bool operator <(const SProjectItem& ItemCmp)
		{
			return (strFilename.compare(ItemCmp.strFilename) == -1);
		}
	};

	vector<SProjectItem> m_vecProjectItems;

	SScFileStatus m_ProjectFileStatus;


protected:

	void ShowModules();
	void ShowProject();
	void ShowClass();
	void ShowProjectFileMenu();
	void OnProjectBeginDrag(NMTREEVIEW* pNmTv);
	void OnProjectEndDrag();
	void OnProjectKeyDown(NMTVKEYDOWN* pKeyDown);

	void UpdateFileClassDisplay(const string& Filename);
	void UpdateProjectClassDisplay(CPdpManager* pProject);
	void UpdateClassDisplay();
	void UpdateFileItemStatus(HTREEITEM hItem);
	void UpdateProjectItemStatus();
	string GetFilename(int ProjFileInx);

	void GetExpandedNodes(HTREEITEM hItem, set<string>& nodes);
	void ExpandNodes(HTREEITEM hItem, set<string>& nodes);
	string GetGroupPath(HTREEITEM hItem);

	HTREEITEM FindFileInTree(HTREEITEM hItemStart, const char* cszFilename);

public:
	//{{AFX_VIRTUAL(CProjectViewBar)
	public:
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CProjectViewBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	afx_msg void OnFilepopupSetBootFile();
	afx_msg void OnFilepopupProperties();
	afx_msg void OnFilepopupAddtoVcs();
	afx_msg void OnFilepopupCheckin();
	afx_msg void OnFilepopupCheckout();
	afx_msg void OnFilepopupUndocheckout();
	afx_msg void OnFilepopupSetGroup();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnFilepopupExplorer();
	afx_msg void OnFilepopupCmdprompt();
	afx_msg void OnRemoveFromProject();
	afx_msg void OnFilepopupDiffFile();
	afx_msg void OnFilepopupSyncFile();
	afx_msg void OnFilepopupFindinProject();
	//}}AFX_MSG
	afx_msg void OnDblclkTree(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROJECTVIEWBAR_H__05D6261E_3F7A_453D_938A_174DD5752E84__INCLUDED_)
