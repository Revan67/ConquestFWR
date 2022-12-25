//////////////////////////////////////////////////////////////////////////////
//  File:    WatchBar.h
//
//  Purpose: This is ia docking window that provides a watch window.
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

#if !defined(AFX_WATCHBAR_H__91B3BB58_58A9_4893_932A_FFB87D04E0B5__INCLUDED_)
#define AFX_WATCHBAR_H__91B3BB58_58A9_4893_932A_FFB87D04E0B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WatchBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWatchBar window
#include "sizecbar.h"
#include "scbarg.h"
#include "VarListCtrl.h"



class CWatchBar : public CSizingControlBarG
{
// Construction
public:
	CWatchBar();
	virtual ~CWatchBar();

	int GetWatchCount();
	void GetWatchItem(int ItemInx, char* szItemLabel, int MaxLen);
	void UpdateWatchItem(int ItemInx, const char* cszItemData);
	void AddWatchItem(const char* cszWatch);
	virtual void LoadState(LPCTSTR lpszProfileName);
    virtual void SaveState(LPCTSTR lpszProfileName);


protected:
	CVarListCtrl m_WatchList;
	//HBRUSH m_hBkBrush;
	CFont m_Font;
	int m_nListCol1Size;


protected:
	
	void OnListDoubleClick();

	//{{AFX_MSG(CWatchBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void OnWatchListDelete();
	void OnWatchEndLabelEdit(int nItem, int nSubItem, const char* cszNewText);
public:
	//{{AFX_VIRTUAL(CWatchBar)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WATCHBAR_H__91B3BB58_58A9_4893_932A_FFB87D04E0B5__INCLUDED_)
