//-----------------------------------------------------------------------------------------------------
// SectorBar
//-----------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "globals.h"

#include "SectorBar.h"
#include "Resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSidebar

CSectorBar::CSectorBar()
{
	m_bits = NULL;
}

//-----------------------------------------------------------------------------------------------------

CSectorBar::~CSectorBar()
{
	if( m_bits )
	{
		delete m_bits;
		m_bits = NULL;
	}
}

//-----------------------------------------------------------------------------------------------------

BEGIN_MESSAGE_MAP(CSectorBar, CSidebar)
	//{{AFX_MSG_MAP(CSectorBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// message handlers

int CSectorBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSidebar::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	const DWORD size = 1204;

	m_bits = new DWORD[ size * size ];
	m_bitmap.CreateBitmap( size, size, 1, 32, m_bits );

	return 0;
}

//-----------------------------------------------------------------------------------------------------

int CSectorBar::DoPaint( CPaintDC& _dc )
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);
	rect.DeflateRect( 1, 1, 1, 1 );

	dc.SelectObject( m_bitmap );
	dc.Draw3dRect( rect, 0x00000000, 0x00000000 );

	return -1;
}

