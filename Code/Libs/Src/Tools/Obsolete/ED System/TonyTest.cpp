// TonyTest.cpp : implementation file
//

#include "PCH.h"
#include "stdafx.h"
#include "ed.h"
#include "TonyTest.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TonyTest dialog


TonyTest::TonyTest(CWnd* pParent /*=NULL*/)
	: CDialog(TonyTest::IDD, pParent)
{
	//{{AFX_DATA_INIT(TonyTest)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void TonyTest::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TonyTest)
	DDX_Control(pDX, IDC_SCROLLBAR1, m_TestScroll);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TonyTest, CDialog)
	//{{AFX_MSG_MAP(TonyTest)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TonyTest message handlers
