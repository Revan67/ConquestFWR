// PropEdit.h : main header file for the PROPEDIT application
//

#if !defined(AFX_PROPEDIT_H__F08D5304_4127_11D3_85B6_0000F4A24553__INCLUDED_)
#define AFX_PROPEDIT_H__F08D5304_4127_11D3_85B6_0000F4A24553__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPropEditApp:
// See PropEdit.cpp for the implementation of this class
//

class CPropEditApp : public CWinApp
{
public:
	CPropEditApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPropEditApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CPropEditApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROPEDIT_H__F08D5304_4127_11D3_85B6_0000F4A24553__INCLUDED_)
