// NewCampaignDlg.cpp : implementation file
//

#include "stdafx.h"
#include "editor.h"
#include "NewCampaignDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNewCampaignDlg dialog


CNewCampaignDlg::CNewCampaignDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewCampaignDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewCampaignDlg)
	m_EditName = _T("");
	//}}AFX_DATA_INIT
}


void CNewCampaignDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewCampaignDlg)
	DDX_Control(pDX, IDC_COMBO_RACE, m_ComboRace);
	DDX_Text(pDX, IDC_EDIT_NAME, m_EditName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewCampaignDlg, CDialog)
	//{{AFX_MSG_MAP(CNewCampaignDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewCampaignDlg message handlers
