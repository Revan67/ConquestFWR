#ifndef  OUTPUT_BAR_H
#define  OUTPUT_BAR_H

#include "Sidebar.h"

struct COutputBar : public CSidebar
{
	virtual ~COutputBar();
	virtual int DoPaint( CPaintDC& );
};

#endif  // OUTPUT_BAR_H
