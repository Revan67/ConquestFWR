//
// CQ2Render.h
//

#include <windows.h>
#include "typedefs.h"
#include <atltypes.h>
#include "globals.h"

typedef S32 ARCHETYPE_INDEX;		// ARCHETYPE_INDEX = index to created archetype
enum OBJID;
template <class Type> class OBJPTR;
#define TOTALLYVOLATILEPTR (0xFFFFFFFF)		// for OBJPTR
#define CQTRACE_H

#include "CQ2Render.h"
#include <TObjRender.h>

bool ArchToMeshObj( ARCHETYPE_INDEX _archID, RenderArch** _renderArch, IMeshObj** _meshObj )
{
	*_renderArch = NULL;
	*_meshObj = NULL;

	RenderArch* renderArch = new RenderArch;
	if( renderArch )
	{
		IMeshObj* meshObj = CreateMeshObj(renderArch,_archID);
		if( meshObj )
		{
			*_renderArch = renderArch;
			*_meshObj = meshObj;
			return true;
		}
	}

	return false;
}