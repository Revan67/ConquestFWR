// ED.h : main header file for the ED application
//

#if !defined(AFX_ED_H__2CB106A9_4341_11D2_823D_0000F4A24556__INCLUDED_)
#define AFX_ED_H__2CB106A9_4341_11D2_823D_0000F4A24556__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CEDApp:
// See ED.cpp for the implementation of this class
//

class CEDApp : public CWinApp
{
	public:
		CEDApp();
		~CEDApp();

	// Overrides
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CEDApp)
	public:
		virtual BOOL InitInstance();
		virtual int ExitInstance();
		virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
	//}}AFX_VIRTUAL

	// Implementation

		//{{AFX_MSG(CEDApp)
			// NOTE - the ClassWizard will add and remove member functions here.
			//    DO NOT EDIT what you see in these blocks of generated code !
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()

	private:
		HACCEL	mAccelerator;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ED_H__2CB106A9_4341_11D2_823D_0000F4A24556__INCLUDED_)
