#pragma once

//
// SystemProps dialog
//
// Edits system properties like name and system kit name
//

#include "SystemStructs.h"

class SystemProps : public CDialog
{
	DECLARE_DYNAMIC(SystemProps)

	friend struct EnumSystemKits;

private:
	System m_data;

public:
	void SetSystemData( System* _system );

public:
	SystemProps(CWnd* pParent = NULL);   // standard constructor
	virtual ~SystemProps();

// Dialog Data
	enum { IDD = IDD_SYSTEMPROPS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonNewstring();
	afx_msg void OnCbnSelchangeComboSyskit();
	virtual BOOL OnInitDialog();
};
