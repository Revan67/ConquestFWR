// VectorPropEdit.cpp : implementation file
//

#include "stdafx.h"
#include "PropEdit.h"
#include "VectorPropEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVectorPropEdit dialog


CVectorPropEdit::CVectorPropEdit(const Property &_p, CWnd* pParent /*=NULL*/)
	: CDialog(CVectorPropEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVectorPropEdit)
	m_PropertyName = _T("");
	m_TypeName = _T("");
	m_VectorX = 0.0f;
	m_VectorY = 0.0f;
	m_VectorZ = 0.0f;
	//}}AFX_DATA_INIT

	p = _p;
	m_TypeName = p.get_type_name ();
	m_PropertyName = p.get_name ();
	if (p.type == PT_VECTOR)
	{
		m_VectorX = p.vectorVal.x;
		m_VectorY = p.vectorVal.y;
		m_VectorZ = p.vectorVal.z;
	}
}


void CVectorPropEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVectorPropEdit)
	DDX_Text(pDX, IDC_NAME, m_PropertyName);
	DDV_MaxChars(pDX, m_PropertyName, 256);
	DDX_Text(pDX, IDC_TYPENAME, m_TypeName);
	DDV_MaxChars(pDX, m_TypeName, 128);
	DDX_Text(pDX, IDC_VECTOR_X, m_VectorX);
	DDX_Text(pDX, IDC_VECTOR_Y, m_VectorY);
	DDX_Text(pDX, IDC_VECTOR_Z, m_VectorZ);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVectorPropEdit, CDialog)
	//{{AFX_MSG_MAP(CVectorPropEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CVectorPropEdit::sync_property ()
{
	// Update the data before proceeding.
	UpdateData ();

	// Store the name and value into the property.
	p.name = m_PropertyName;
	PersistVector v (m_VectorX, m_VectorY, m_VectorZ);
	p.set_vector(&v);
}

/////////////////////////////////////////////////////////////////////////////
// CVectorPropEdit message handlers

void CVectorPropEdit::OnOK() 
{
	// TODO: Add extra validation here
	sync_property ();
	
	CDialog::OnOK();
}

BOOL CVectorPropEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	// If the type is not a vector, punt.
	if (p.type != PT_VECTOR)
	{
		EndDialog (IDCANCEL);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
