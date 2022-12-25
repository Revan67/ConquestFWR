// ITemplate
//
//
//

#ifndef ITemplate_H
#define ITemplate_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_ITemplate = "ITemplate";

dacom_interface( ITemplate )
{
	DACOM_INTERFACE_METHOD( Method,	(void));
};

#endif
