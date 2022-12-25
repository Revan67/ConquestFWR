// NewGroup.cpp : implementation file
//

#include "stdafx.h"
#include "Editor.h"
#include "NewGroup.h"


// NewGroup dialog

IMPLEMENT_DYNAMIC(NewGroup, CDialog)
NewGroup::NewGroup(CWnd* pParent /*=NULL*/)
	: CDialog(NewGroup::IDD, pParent)
{
}

NewGroup::~NewGroup()
{
}

void NewGroup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(NewGroup, CDialog)
	ON_EN_CHANGE(IDC_EDIT_NAME, OnEnChangeEditName)
	ON_EN_CHANGE(IDC_RICHEDIT, OnEnChangeRichedit)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()


// NewGroup message handlers

void NewGroup::OnEnChangeEditName()
{
}

void NewGroup::OnEnChangeRichedit()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

BOOL NewGroup::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRichEditCtrl* ctrl = (CRichEditCtrl*)GetDlgItem(IDC_RICHEDIT);
	ctrl->SetEventMask( ENM_CHANGE );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void NewGroup::OnBnClickedOk()
{
	CRichEditCtrl* ctrl = (CRichEditCtrl*)GetDlgItem(IDC_RICHEDIT);

	TCHAR buffer[64];
	ctrl->GetLine(0, buffer, countof(buffer)-1 );

	TCHAR* newLine = strchr(buffer,_T(13));
	while( newLine )
	{
		*newLine = 0;
		newLine++;
		newLine = strchr(newLine,_T(13));
	}

	newGroupName = buffer;

	// TODO: Add your control notification handler code here
	OnOK();
}

void NewGroup::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}
