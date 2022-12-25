// Editor.h : main header file for the EDITOR application
//

#if !defined(AFX_EDITOR_H__FAC6EFDF_2E14_4944_88D0_510BA2B48592__INCLUDED_)
#define AFX_EDITOR_H__FAC6EFDF_2E14_4944_88D0_510BA2B48592__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "Splash.h"

/////////////////////////////////////////////////////////////////////////////
// CEditorApp:
// See Editor.cpp for the implementation of this class
//

class CEditorApp : public CWinApp
{
public:
	CString path;

private:

public:
	CEditorApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditorApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	virtual BOOL OnIdle(LONG lCount);
	virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CEditorApp)
	afx_msg void OnAppAbout();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

struct System;
struct IObject;

namespace Editor
{
	bool GetOpenFileList( CArray<CString,CString>& _array );
	bool GetSaveFile( CString& _copyFilename );
	BOOL LoadBMPImage( LPCTSTR sBMPFile, CBitmap& bitmap, CPalette *pPal );
	bool PushFileDirectory( CString& _string );

	extern CRect  testRect1;
	extern CRect  testRect2;
	extern float  deltaTime; // in seconds
	extern SPLASH g_Splash;

	System* GetActiveSystem();
	System* GetSystem( U32 _sectorID, U32 _systemID );

	IObject* GetObjectByHandle( const char* _scriptHandle );
}

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITOR_H__FAC6EFDF_2E14_4944_88D0_510BA2B48592__INCLUDED_)
