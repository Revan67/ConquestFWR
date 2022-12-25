// VoiceWin.h : main header file for the VOICEWIN application
//

#if !defined(AFX_VOICEWIN_H__49379818_9EE3_11D1_8060_0000F4A24526__INCLUDED_)
#define AFX_VOICEWIN_H__49379818_9EE3_11D1_8060_0000F4A24526__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CVoiceWinApp:
// See VoiceWin.cpp for the implementation of this class
//

class CVoiceWinApp : public CWinApp
{
public:
	CVoiceWinApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVoiceWinApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CVoiceWinApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOICEWIN_H__49379818_9EE3_11D1_8060_0000F4A24526__INCLUDED_)
