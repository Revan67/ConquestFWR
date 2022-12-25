#if !defined(AFX_NEWCAMPAIGNDLG_H__9D9A8771_02F9_4AF5_96FD_77F59612403C__INCLUDED_)
#define AFX_NEWCAMPAIGNDLG_H__9D9A8771_02F9_4AF5_96FD_77F59612403C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewCampaignDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewCampaignDlg dialog

class CNewCampaignDlg : public CDialog
{
// Construction
public:
	CNewCampaignDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewCampaignDlg)
	enum { IDD = IDD_NEW_CAMPAIGN };
	CComboBox	m_ComboRace;
	CString	m_EditName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewCampaignDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewCampaignDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWCAMPAIGNDLG_H__9D9A8771_02F9_4AF5_96FD_77F59612403C__INCLUDED_)
