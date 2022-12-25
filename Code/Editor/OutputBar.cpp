// OutputBar

#include "stdafx.h"
#include "globals.h"

#include "OutputBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------------------------------

COutputBar::~COutputBar()
{
}

//-----------------------------------------------------------------------------------------------------

int COutputBar::DoPaint( CPaintDC& )
{
	return -1;
}

