//////////////////////////////////////////////////////////////////////////////
//  File:    DbgRemoteDoc.h
//
//  Purpose: Interface of the CDbgRemoteDoc class - which holds a single 
//  python source file.
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
#if !defined(AFX_DBGREMOTEDOC_H__52BCA9D6_7126_4181_A045_4A464BA855CF__INCLUDED_)
#define AFX_DBGREMOTEDOC_H__52BCA9D6_7126_4181_A045_4A464BA855CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDbgRemoteDoc : public CDocument
{
protected:	// create from serialization only

	CDbgRemoteDoc();
	DECLARE_DYNCREATE(CDbgRemoteDoc)
	virtual ~CDbgRemoteDoc();

public:		// types

	enum UpdateHint
	{
		HINT_DOC_OPENED,
		HINT_UPDATESTYLE,
		HINT_DOC_REOPENED,
		HINT_DOC_SAVED
	};

public:		// methods

	const CString& GetFileName();
	bool ReadOnly() const;

	void AddBreakpoint(int nLine);         // Added 10-18-02 Bruce Dickey
	void RemoveBreakpoint(int nLine);      // Added 10-18-02 Bruce Dickey
	void RemoveAllBreakpointsByFile(void); // Added 10-19-02 Bruce Dickey
	void ToggleBreakpoint(int nLine);
	void UpdateBreakpoints(const char* cszBrkptList);

	bool HasChangedOnDisk()const;

	void* GetSciDocPtr();
	void SetSciDocPtr(void* pDoc);

private:	// methods

	bool GetEditorText(char_vector& fileData);
	void StoreFileModTime(const char* cszFilePath);
public:		// CDocument overrides

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// ClassWizard generated virtual function overrides

	//{{AFX_VIRTUAL(CDbgRemoteDoc)
	public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual void SetModifiedFlag(BOOL bModified = TRUE);
	//}}AFX_VIRTUAL

protected:	// Generated message map functions

	//{{AFX_MSG(CDbgRemoteDoc)
	afx_msg void OnDebugTestCompile();
	afx_msg void OnFileSave();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:	// data

	CString m_strFileName;

	FILETIME m_LastModifiedTime;

	void* m_pSciDoc;
public:
	afx_msg void OnFileSaveAs();
	void ShowProperties(void);
};

inline const CString& CDbgRemoteDoc::GetFileName()
{
	return m_strFileName;
}

//{{AFX_INSERT_LOCATION}}

#endif // !defined(AFX_DBGREMOTEDOC_H__52BCA9D6_7126_4181_A045_4A464BA855CF__INCLUDED_)
