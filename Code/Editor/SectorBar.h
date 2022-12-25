#ifndef  SECTOR_BAR_H
#define  SECTOR_BAR_H

#include "Sidebar.h"

class CSectorBar : public CSidebar
{
public:
	CSectorBar();
	virtual ~CSectorBar();
	virtual DoPaint( CPaintDC& );

private:
	CBitmap m_bitmap;
	DWORD*  m_bits;

	// Generated message map functions
protected:
	//{{AFX_MSG(CSectorBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  // SECTOR_BAR_H
