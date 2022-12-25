// IDACOMEngineInstance
//
// Temporary interface
//

#ifndef IDACOMEngineInstance_H
#define IDACOMEngineInstance_H

#include "DACOM.h"
#include "engine.h"
#include "Mesh.h"

#include "DACOM_Utility.h"

static const char *IID_IDACOMEngineInstance = "IDACOMEngineInstance";

dacom_interface( IDACOMEngineInstance )
{
	DACOM_INTERFACE_METHOD( GetMesh,			(Mesh **ppmesh));
	DACOM_INTERFACE_METHOD( GetInstanceIndex,	(INSTANCE_INDEX *pidx));
};

#endif
