//////////////////////////////////////////////////////////////////////////////
//  File:    varlistctrl.h
//
//  Purpose: This is a custom list control used by the watch window that provides
//  the ability to modify both the first and second columns.
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

#if !defined(AFX_VARLISTCTRL_H__8601422E_1698_42D2_9E9A_3285822CDC89__INCLUDED_)
#define AFX_VARLISTCTRL_H__8601422E_1698_42D2_9E9A_3285822CDC89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// VarListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVarListCtrl window

class CVarListCtrl : public CListCtrl
{
// Construction
public:
	CVarListCtrl();
	virtual ~CVarListCtrl();
	
	BOOL EditEnabled(int nCol);
	void EnableEdit(int nCol);
	CEdit* EditSubLabel(int nItem, int nCol);

protected:
	
	int HitTestEx(CPoint &point, int *col) const;
	
	
	//void PositionHeader();

protected:
	DWORD m_dwColEditEnableMask;


	//{{AFX_VIRTUAL(CVarListCtrl)
	protected:
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL
protected:
	//{{AFX_MSG(CVarListCtrl)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CVarListEdit window

class CVarListEdit : public CEdit
{
// Construction
public:
	CVarListEdit(int iItem, int iSubItem, CString sInitText);


// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVarListEdit)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVarListEdit();

private:
	int m_iItem;
	int m_iSubItem;
	CString m_sInitText;
	BOOL    m_bESC;	 	// To indicate whether ESC key was pressed


	// Generated message map functions
protected:

	//{{AFX_MSG(CVarListEdit)
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnNcDestroy();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VARLISTCTRL_H__8601422E_1698_42D2_9E9A_3285822CDC89__INCLUDED_)
