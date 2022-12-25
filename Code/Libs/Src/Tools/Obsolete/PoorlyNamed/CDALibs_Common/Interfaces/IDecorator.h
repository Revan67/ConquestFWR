// IDecorator
//
//
//

#ifndef IDecorator_H
#define IDecorator_H

#include "DACOM.h"
#include "extent.h"

#include "DACOM_Utility.h"

static const char *IID_IDecorator = "IDecorator";

dacom_interface( IDecorator )
{
	DACOM_INTERFACE_METHOD( SetDecorated,		(IDAComponent  *idacomponent));
	DACOM_INTERFACE_METHOD( GetDecorated,		(IDAComponent **idacomponent));
/*												
#define SF_SHOW_NAMES			(1<<0)
#define SF_SHOW_FACES			(1<<1)
#define SF_SHOW_EDGES			(1<<2)
#define SF_SHOW_VERTICES		(1<<3)
#define SF_SHOW_FACE_NORMALS	(1<<4)
#define SF_SHOW_VERTEX_NORMALS	(1<<5)
#define SF_SHOW_TEXTURES		(1<<6)
#define SF_SHOW_AXIS			(1<<7)

	DACOM_INTERFACE_METHOD( Show,				( U32 flag, U32 yesno ));
	DACOM_INTERFACE_METHOD( ShowExtents,		( ExtentType type, U32 yesno ));
*/
};

#endif
