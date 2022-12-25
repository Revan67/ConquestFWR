// SelectEntityPart.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "SelectEntityPart.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectEntityPart dialog


CSelectEntityPart::CSelectEntityPart(ROS::ASceneEntity &entity, CWnd* pParent /*=NULL*/)
: CDialog(CSelectEntityPart::IDD, pParent), selectedPart (CEntityTree::kNoType, ""), bound_entity(entity)
{
	//{{AFX_DATA_INIT(CSelectEntityPart)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSelectEntityPart::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectEntityPart)
	DDX_Control(pDX, IDC_PART_TREE, mPartTree);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectEntityPart, CDialog)
	//{{AFX_MSG_MAP(CSelectEntityPart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectEntityPart message handlers

void CSelectEntityPart::OnOK() 
{
	// Store the selected part name and type into public variables.

	selectedPart = mPartTree.selectedPart;

	CDialog::OnOK();
}

BOOL CSelectEntityPart::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	mPartTree.bind_to_entity (bound_entity);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
