// VoiceWinDlg.h : header file
//

#if !defined(AFX_VOICEWINDLG_H__4937981A_9EE3_11D1_8060_0000F4A24526__INCLUDED_)
#define AFX_VOICEWINDLG_H__4937981A_9EE3_11D1_8060_0000F4A24526__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "VoiceNet.h"

/////////////////////////////////////////////////////////////////////////////
// CVoiceWinDlg dialog

class CVoiceWinDlg : public CDialog
{
// Construction
public:
	CVoiceWinDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CVoiceWinDlg)
	enum { IDD = IDD_VOICEWIN_DIALOG };
	CEdit	m_messageBar;
	CEdit	m_remoteHost;
	CEdit	m_remotePort;
	CEdit	m_localPort;
	CEdit	m_localHost;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVoiceWinDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CVoiceWinDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBreak();
	afx_msg void OnInitiate();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	VoiceNet* voiceNet;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VOICEWINDLG_H__4937981A_9EE3_11D1_8060_0000F4A24526__INCLUDED_)
