// ISceneCamera
//
//
//

#ifndef ISceneCamera_H
#define ISceneCamera_H

#include "DACOM.h"
#include "DACOM_Utility.h"

static const char *IID_ISceneCamera = "ISceneCamera";


typedef enum {
	ISC_ST_INVALID		=0,		// 
	ISC_ST_APEX			=1,		// A,  B
	ISC_ST_APEX_CLOSEUP	=2,		// A,  B
	ISC_ST_EXTERNAL		=3,		// A,  B,  [C]
	ISC_ST_INTERNAL		=4,		// A, [B]
	ISC_ST_TRACK		=5,		// A,  B,  min_d,  max_d
	ISC_ST_PAN			=6,		// A,  B,  min_d,  max_d
	ISC_ST_FOLLOW		=7,		// A,  B,  min_d,  max_d
	ISC_ST_FIXED		=8,		// eye, at, up, fov, etc...
} ISCSHOTTYPE;

struct ISCSHOTDESC
{
public:
	ISCSHOTDESC( )
	{
		shot_type = ISC_ST_INVALID;
	}

	//

	ISCSHOTDESC( ISCSHOTTYPE type, IDAComponent *A, IDAComponent *B=NULL, IDAComponent *C=NULL )
	{
		shot_type = type;
		actors[0] = A;
		actors[1] = B;
		actors[2] = C;
		min_distance = 0.0;
		max_distance = 0.0;
	}
	
	//

	ISCSHOTDESC( ISCSHOTTYPE type, IDAComponent *A, IDAComponent *B, float min_d, float max_d )
	{
		shot_type = type;
		actors[0] = A;
		actors[1] = B;
		min_distance = min_d;
		max_distance = max_d;
	}
	
	//

	ISCSHOTDESC( Vector &e, Vector &a, Vector &u, float _fov_x, float _aspect )
	{
		shot_type = ISC_ST_FIXED;
		eye = e;
		at = a;
		world_up = u;
		fov_x = _fov_x;
		aspect = _aspect;
	}
	
	//


public:
	ISCSHOTTYPE		shot_type;
	union {
		struct {
			IDAComponent   *actors[3];
			float			min_distance;
			float			max_distance;
		};
		struct {
			Vector			eye;
			Vector			at;
			Vector			world_up;
			float			fov_x;
			float			aspect;
		};
	};
};

dacom_interface( ISceneCamera )
{
	DACOM_INTERFACE_METHOD( BeginScene,					( void ));
	DACOM_INTERFACE_METHOD( EndScene,					( void ));

	DACOM_INTERFACE_METHOD( SetShot,					( ISCSHOTDESC &shot_desc ));
	DACOM_INTERFACE_METHOD( GetShot,					( ISCSHOTDESC &shot_desc ));
};

#endif
