// SelectEntityTypeDialog.cpp : implementation file
//

#include <assert.h>
#include "stdafx.h"
#include "SelectEntityTypeDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectEntityTypeDialog dialog


CSelectEntityTypeDialog::CSelectEntityTypeDialog(CWnd* pParent /*=NULL*/)
: CDialog(CSelectEntityTypeDialog::IDD, pParent), mEntityType(kDeformable)
{
	//{{AFX_DATA_INIT(CSelectEntityTypeDialog)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSelectEntityTypeDialog::EntityType CSelectEntityTypeDialog::GetSelection() const
{
	return mEntityType;
}

void CSelectEntityTypeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectEntityTypeDialog)
	DDX_Control(pDX, IDC_DEFORMABLE_ENTITY_RADIO, mDeformableEntityRadio);
	DDX_Control(pDX, IDC_COMPOUND_ENTITY_RADIO, mCompoundEntityRadio);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectEntityTypeDialog, CDialog)
	//{{AFX_MSG_MAP(CSelectEntityTypeDialog)
	ON_BN_CLICKED(IDC_COMPOUND_ENTITY_RADIO, OnCompoundEntityRadio)
	ON_BN_CLICKED(IDC_DEFORMABLE_ENTITY_RADIO, OnDeformableEntityRadio)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectEntityTypeDialog message handlers

BOOL CSelectEntityTypeDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	assert(mEntityType == kDeformable);

	mDeformableEntityRadio.SetCheck(1);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectEntityTypeDialog::OnCompoundEntityRadio() 
{
	mEntityType = kCompound;	
}

void CSelectEntityTypeDialog::OnDeformableEntityRadio() 
{
	mEntityType = kDeformable;	
}
