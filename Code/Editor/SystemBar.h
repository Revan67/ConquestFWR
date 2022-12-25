#ifndef  SYSTEM_BAR_H
#define  SYSTEM_BAR_H

#include "Sidebar.h"

struct CSystemBar : public CSidebar
{
	virtual ~CSystemBar();
	virtual DoPaint( CPaintDC& );
};

#endif  // SYSTEM_BAR_H
