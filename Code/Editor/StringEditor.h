#pragma once


// StringEditor dialog

class StringEditor : public CDialog
{
	DECLARE_DYNAMIC(StringEditor)

private:
	U32 m_selectedStringID;

public:
    U32 GetSelectedString()
	{
		return m_selectedStringID;
	}

    void SetSelectedString( U32 _stringID )
	{
		m_selectedStringID = _stringID;
	}

public:
	StringEditor(CWnd* pParent = NULL);   // standard constructor
	virtual ~StringEditor();

// Dialog Data
	enum 
	{ 
		IDD = IDD_STRING_TABLE,
		INVALID_STRING = -1,
	};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	void initStrings();

private:
	CRect ms_mainRect;
	CRect ms_buttonOk;
	CRect ms_buttonCancel;
	CRect ms_stringTable;
	CRect ms_buttonDelete;
	CRect ms_buttonEdit;
	CRect ms_buttonNew;
	U32   ms_stringLowestID;

private:
	afx_msg void    OnBnClickedOk();
	afx_msg void    OnBnClickedCancel();
	afx_msg void    OnSize(UINT nType, int cx, int cy);
	afx_msg int     OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg LRESULT InitDialog(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedButtonDelete();
	afx_msg void OnBnClickedButtonEdit();
	afx_msg void OnBnClickedButtonNew();
};

