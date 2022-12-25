#if !defined(AFX_TONYTEST_H__70CC1482_6397_11D2_85B3_0000F4A24553__INCLUDED_)
#define AFX_TONYTEST_H__70CC1482_6397_11D2_85B3_0000F4A24553__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TonyTest.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// TonyTest dialog

class TonyTest : public CDialog
{
// Construction
public:
	TonyTest(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(TonyTest)
	enum { IDD = IDD_TONYTEST };
	CScrollBar	m_TestScroll;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TonyTest)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TonyTest)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TONYTEST_H__70CC1482_6397_11D2_85B3_0000F4A24553__INCLUDED_)
