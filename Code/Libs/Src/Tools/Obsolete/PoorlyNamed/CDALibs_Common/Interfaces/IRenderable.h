// IRenderable
//
//
//

#ifndef IRenderable_H
#define IRenderable_H

#include "DACOM.h"
#include "DACOM_Utility.h"
#include "ILowLevelCamera.h"


static const char *IID_IRenderable = "IRenderable";

dacom_interface( IRenderable )
{
	DACOM_INTERFACE_METHOD( Render,	(ILowLevelCamera *IC));
};

#endif
