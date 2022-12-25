#pragma once


// NewGroup dialog

class NewGroup : public CDialog
{
	DECLARE_DYNAMIC(NewGroup)

public:
	NewGroup(CWnd* pParent = NULL);   // standard constructor
	virtual ~NewGroup();

// Dialog Data
	enum { IDD = IDD_NEW_GROUP };

public:
	CString newGroupName;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEditName();
	afx_msg void OnEnChangeRichedit();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
