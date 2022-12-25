// SystemBar

#include "stdafx.h"
#include "globals.h"

#include "SystemBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-----------------------------------------------------------------------------------------------------

CSystemBar::~CSystemBar()
{
}

//-----------------------------------------------------------------------------------------------------

int CSystemBar::DoPaint( CPaintDC& )
{
	return -1;
}

