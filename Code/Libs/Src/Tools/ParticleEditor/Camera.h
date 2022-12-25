// Loose Cannon Productions
// camera.h begun July 23, 1997 - TZ

#ifndef CAMERA_H
#define CAMERA_H

#include <stdio.h>

#include "3dmath.h"
#include "engine.h"
#include "renderer.h"
#include "view2d.h"
#include "ICamera.h"
#include "rendpipeline.h"

//

enum ProjectionMode
{
	PM_PERSPECTIVE,
	PM_ORTHO_XY,
	PM_ORTHO_YZ,
	PM_ORTHO_XZ

};

//

void transformTo4x4(SINGLE m[16],const Transform &t);

class Camera {
	
	static INSTANCE_INDEX	index;				// Instance index of camera
	static Vector			position;			// Camera position
	static Matrix			orientation;		// Camera orientation
	
	static ProjectionMode	projection_mode;	// Projection mode of camera:

	static IRenderPipeline  *renderpipeline;

	public:

	static void		init( IRenderPipeline *pipe, IEngine *engine, ViewRect *rect );
	static void		uninit();

	static void		set_projection_mode(ProjectionMode pm);

	static Matrix	getOrientation();
	static void		setOrientation(Matrix *m);
	static void		rotate(S32 axis, SINGLE angle);
	static void		rotateRelative(SINGLE ex, SINGLE ey, SINGLE ez);

	static Vector	getPosition();
	static void		setPosition(Vector *v);
	static void		setPosition(SINGLE px,SINGLE py,SINGLE pz);
	
	static void		movePosition(const Vector &v);
	static void		movePosition(SINGLE dx, SINGLE dy, SINGLE dz);
	static void		moveRelative(SINGLE dx, SINGLE dy, SINGLE dz);
	static void		moveZRelative(SINGLE dx, SINGLE dy, SINGLE dz);

	static Transform	getTransform();
	static void			setTransform(Transform *t);
	
    static INSTANCE_INDEX getIndex();
    
    static struct ICamera *getCamera();
	
    static SINGLE   get_far_plane_distance(void);
    static void     set_far_plane_distance(SINGLE new_z);

    static void		lookAt( SINGLE, SINGLE, SINGLE );   // look at point in 3 space

	static PSRESULT point_to_screen(Vector world_point, S32 *sx, S32 *sy, SINGLE *inz = 0);
	
	static void		screen_to_point(S32 screen_x, S32 screen_y, Vector *result, SINGLE specified_z = 0.0);
	
	static BOOL32	point_in_poly(ViewPoint point, const ViewPoint * verts, int n);

	static void		initializeMatrix();

    static void     setFOV( SINGLE fovAngle );

	static void		setNearPlaneDistance(SINGLE distance);
	static void		setFarPlaneDistance(SINGLE distance);

	};


#endif
