// ISimulatable
//
//
//

#ifndef ISimulatable_H
#define ISimulatable_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_ISimulatable = "ISimulatable";

dacom_interface( ISimulatable )
{
	DACOM_INTERFACE_METHOD( Import,	(void));
	DACOM_INTERFACE_METHOD( Update,	(void));
};

#endif
