// IDACOMClient
//
//
//

#ifndef IDACOMClient_H
#define IDACOMClient_H

#include "DACOM.h"
#include "engine.h"
#include "system.h"

#include "DACOM_Utility.h"

static const char *IID_IDACOMClient = "IDACOMClient";

dacom_interface( IDACOMClient )
{
	DACOM_INTERFACE_METHOD( SetIEngine,		( IEngine *iengine));
	DACOM_INTERFACE_METHOD( SetISystem,		( ISystemContainer *isystem));
	DACOM_INTERFACE_METHOD( SetICOManager,	( ICOManager *icomanager));
};

#endif
