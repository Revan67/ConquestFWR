// SimplePropEdit.cpp : implementation file
//

#include "stdafx.h"
#include "PropEdit.h"
#include "SimplePropEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSimplePropEdit dialog


CSimplePropEdit::CSimplePropEdit(const Property &_p, CWnd* pParent /*=NULL*/)
	: CDialog(CSimplePropEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSimplePropEdit)
	m_PropertyName = _T("");
	m_TypeName = _T("");
	m_PropertyValue = _T("");
	//}}AFX_DATA_INIT

	p = _p;
	m_TypeName = p.get_type_name ();
	m_PropertyName = p.get_name ();
	if (p.type == PT_STRING)
	{
		m_PropertyValue = (const char *) p.get_data ();
	}
	else
	{
		m_PropertyValue = p.get_data_string ();
	}
}


void CSimplePropEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSimplePropEdit)
	DDX_Text(pDX, IDC_NAME, m_PropertyName);
	DDV_MaxChars(pDX, m_PropertyName, 128);
	DDX_Text(pDX, IDC_TYPENAME, m_TypeName);
	DDV_MaxChars(pDX, m_TypeName, 128);
	DDX_Text(pDX, IDC_VALUE, m_PropertyValue);
	DDV_MaxChars(pDX, m_PropertyValue, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSimplePropEdit, CDialog)
	//{{AFX_MSG_MAP(CSimplePropEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSimplePropEdit message handlers

void CSimplePropEdit::OnOK() 
{
	// TODO: Add extra validation here

	// Valid date the numbers.
	switch (p.type)
	{
	case PT_LONG:
		// Valid characters are 0-9,-
		{
			m_PropertyValue = m_PropertyValue.SpanIncluding ("0123456789-");
			if (m_PropertyValue.IsEmpty())
			{
				// Invalid data, so simply return.
				// *** TODO: Insert a message box here.
				return;
			}
			sync_property ();
		}
		break;

	case PT_ULONG:
		// Valid characters are 0-9
		{
			m_PropertyValue = m_PropertyValue.SpanIncluding ("0123456789");
			if (m_PropertyValue.IsEmpty())
			{
				// Invalid data, so simply return.
				// *** TODO: Insert a message box here.
				return;
			}
			sync_property ();
		}
		break;

	case PT_SINGLE:
	case PT_DOUBLE:
		// Valid characters are 0-9,-,+,e,.
		{
			m_PropertyValue = m_PropertyValue.SpanIncluding ("0123456789-+e.");
			if (m_PropertyValue.IsEmpty())
			{
				// Invalid data, so simply return.
				// *** TODO: Insert a message box here.
				return;
			}
			sync_property ();
		}
		break;

	case PT_STRING:
		// All characters are valid, so don't filter at all.
		sync_property ();
		break;

	default:
		// You should never get here.
		ASSERT ("Invalid property type for simple property edit dialog" && 0);
		break;
	}

	CDialog::OnOK();
}

void CSimplePropEdit::OnCancel() 
{
	// TODO: Add extra cleanup here
	
	CDialog::OnCancel();
}

BOOL CSimplePropEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSimplePropEdit::sync_property ()
{
	// Update the data before proceeding.
	UpdateData ();

	// Store the name and value into the property.
	p.name = m_PropertyName;
	p.set(p.type, m_PropertyValue);
}

