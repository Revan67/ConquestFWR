
#include <windows.h>

#include "basecam.h"

#include "camera.h"

INSTANCE_INDEX	 Camera::index;
Vector			 Camera::position;
ProjectionMode	 Camera::projection_mode;
Matrix			 Camera::orientation;
IRenderPipeline *Camera::renderpipeline = NULL;

//

static  BaseCamera *camera=NULL;

//

void Camera::init( IRenderPipeline *pipe, IEngine *engine, ViewRect *rect )
{
	if( (renderpipeline = pipe) == NULL ) {
		return;
	}

	renderpipeline->AddRef();

	orientation.set_identity();
	position.set(0.0,0.0,150.0);
	
	renderpipeline->set_perspective( 45.0, 4.0 / 3.0, 1.0, 1e5 );

	camera = new BaseCamera( engine, rect );

	index = camera->index;
	camera->set_far_plane_distance(1e4);
	camera->set_orientation( orientation );
    camera->set_position( position );
}

//

void Camera::setNearPlaneDistance(SINGLE distance)
{
	if (camera)
		camera->set_near_plane_distance(distance);
}

//

void Camera::setFarPlaneDistance(SINGLE distance)
{
	if (camera)
		camera->set_far_plane_distance(distance);
}

//

void Camera::initializeMatrix()
{
	if( renderpipeline == NULL ) {
		return ;
	}

	Transform	xform;

	switch (projection_mode)
	{
		case PM_PERSPECTIVE:
			xform = getTransform();
			renderpipeline->set_modelview(xform.get_inverse());
			renderpipeline->set_perspective (camera->fovy, camera->aspect, camera->znear, camera->zfar);
			break;
	
		case PM_ORTHO_XY:
			xform.set_identity();
			renderpipeline->set_modelview (xform);
			renderpipeline->set_ortho (-100, 100, 100, -100, 0, 100);
			break;
	}
}

//

void Camera::uninit()
{
	if( renderpipeline ) {
		renderpipeline->Release();
		renderpipeline = NULL;
	}

    delete camera;
    camera = NULL;
}

//

void Camera::set_projection_mode(ProjectionMode pm)
{
	projection_mode = pm;

	initializeMatrix();
}

//

INSTANCE_INDEX Camera::getIndex()
{
    return index;
}

//

struct ICamera *Camera::getCamera()
{
    return camera;
}

//

Matrix Camera::getOrientation()
{
	return orientation;
}

//

void Camera::setOrientation(Matrix *m)
{
	camera->set_orientation(*m);
	orientation = *m;
	
}

//

Vector Camera::getPosition()
{
	return position;
}

//

void Camera::setPosition(Vector *v)
{
	camera->set_position(*v);
    position = *v;
    
}

//

void Camera::setPosition(SINGLE px,SINGLE py,SINGLE pz)
{
	Vector v(px,py,pz);

	setPosition(&v);
	
}

//

void Camera::movePosition(const Vector &v)
{
	position += v;
    camera->set_position(position);

}

//

void Camera::movePosition(SINGLE dx, SINGLE dy, SINGLE dz)
{
	Vector v(dx, dy, dz);

	movePosition(v);
}

//

void Camera::moveRelative(SINGLE dx, SINGLE dy, SINGLE dz)
{
	Vector v(dx, dy, dz);

	v = orientation * v;

	movePosition(v);
}

//

void Camera::moveZRelative(SINGLE dx, SINGLE dy, SINGLE dz)
{
	// Y-only version of moveRelative (for the editor):

	Vector v;

	v.x = (orientation.d[0][0] * dx) + (orientation.d[0][1] * dy);
	v.y = (orientation.d[1][0] * dx) + (orientation.d[1][1] * dy);
	v.z = 0.0;

	movePosition(v);
}

//

void Camera::rotate(S32 rotationAxis, SINGLE angle)
{
	Matrix newOrientation;
	Quaternion matrixQuat(orientation);
	Vector axis;
	Transform t = getTransform();
	
	if (rotationAxis == X_AXIS)
	{
		axis.set(1.0, 0.0, 0.0);
	}
	else
	if (rotationAxis == Y_AXIS)
	{
		axis.set(0.0, 1.0, 0.0);
	}
	else
	{
		axis.set(0.0, 0.0, 1.0);
	}

	Quaternion rotation(axis, angle);
	MATH_ENGINE()->quaternion_to_matrix(newOrientation, matrixQuat * rotation); 
	setOrientation(&newOrientation);

}

//

void Camera::rotateRelative(SINGLE ex, SINGLE ey, SINGLE ez)
{
	// View-relative euler-angle rotation:
	
	Vector x_axis, y_axis;
	Matrix xMatrix, yMatrix;

	x_axis.set(0.0, 0.0, 1.0);
	y_axis.set(1.0, 0.0, 0.0);
	
	Quaternion qx(x_axis, ex);
	MATH_ENGINE()->quaternion_to_matrix(xMatrix, qx);

	Quaternion qy(y_axis, ey);
	MATH_ENGINE()->quaternion_to_matrix(yMatrix, qy);

	xMatrix *= yMatrix;
	
	setOrientation(&xMatrix);

}

//

Transform Camera::getTransform()
{
	Transform t;

	t.set_orientation(orientation);
	t.set_position(position);

	return t;
}

//

void Camera::setTransform(Transform *t)
{
	camera->set_transform(*t);
    
    position    = t->get_position();
    orientation = t->get_orientation();
		
}

//

void CrossProduct(Vector *u,Vector *v,Vector *normal)
{
	// The cross product is useful for figuring a normal to a given plane.
	//
	// |C|=|A|*|B| sin(angle)
	//
	// where A, B, and C are vectors and "angle" is the angle between A and B.  C is the perpendicular vector.

	normal->x= (u->y*v->z-u->z*v->y);
	normal->y=-(u->x*v->z-u->z*v->x);
	normal->z= (u->x*v->y-u->y*v->x);
}

//

// Sets up camera to look at point in 3 space
// note - this assumes left-handed system (z is up)
void Camera::lookAt( SINGLE x, SINGLE y, SINGLE z )
{
    Vector look;
    Vector right, up;
    Vector worldUp;

    worldUp.set( 0.0, 0.0, 1.0 );

    look.x = position.x - x;
    look.y = position.y - y;
    look.z = position.z - z;

    // move x to the side a little so camera doesn't snap off at this position
    if( look.y == 0.0 && look.x == 0.0 ) look.x += .00001;
    look.normalize();

    CrossProduct( &worldUp, &look, &right );
    CrossProduct( &look, &right, &up );

    right.normalize();
    up.normalize();

    Matrix m;       
    m.d[0][0] = right.x;
    m.d[1][0] = right.y;
    m.d[2][0] = right.z;

    m.d[0][1] = up.x;
    m.d[1][1] = up.y;
    m.d[2][1] = up.z;

    m.d[0][2] = look.x;
    m.d[1][2] = look.y;
    m.d[2][2] = look.z;

    setOrientation( &m );
}

//

PSRESULT Camera::point_to_screen(Vector world, S32 *sx, S32 *sy, SINGLE *zdepth)
{
    SINGLE screen_x = *sx;
    SINGLE screen_y = *sy;

    bool   cresult = false;
    
    SINGLE temp2;
    SINGLE &temp = (zdepth) ? *zdepth : temp2;

    if (camera->point_to_screen(screen_x, screen_y, temp, world))
    {
        *sx = screen_x;
        *sy = screen_y;

        return PS_VALID;
    }
    else
    {
        *sx = 0;
        *sy = 0;
        return PS_OFF_PANE;
    }
}

//

void Camera::screen_to_point(S32 screen_x, S32 screen_y, Vector *result, SINGLE specified_z)
{
	// Return the point of intersection of a ray through the screen (x, y) coordinate through the plane
	// of z = specified_z)

    SINGLE sx = screen_x;
    SINGLE sy = screen_y;

    camera->screen_to_point(*result, sx, sy);
    
    // Find camera vector through near plane:

    Vector cvec = *result;
	cvec.normalize();

    if (cvec.z == 0.0)
    {
        return;
    }

	// Find intersection with z = specified plane:

	SINGLE mfactor = (position.z - specified_z) / -cvec.z;
	cvec *= mfactor;

	*result = position + cvec;
	
}

//

void transformTo4x4(SINGLE m[16],const Transform &t)
{
    m[ 0] = t.d[0][0];
    m[ 1] = t.d[1][0];
    m[ 2] = t.d[2][0];
    m[ 3] = 0;

    m[ 4] = t.d[0][1];
    m[ 5] = t.d[1][1];
    m[ 6] = t.d[2][1];
    m[ 7] = 0;

    m[ 8] = t.d[0][2];
    m[ 9] = t.d[1][2];
    m[10] = t.d[2][2];
    m[11] = 0;

    m[12] = t.translation.x;
    m[13] = t.translation.y;
    m[14] = t.translation.z;
    m[15] = 1;
}

//

BOOL32 Camera::point_in_poly(ViewPoint point, const ViewPoint * verts, int n)
{
	ViewPoint shifted[64];

	const ViewPoint * src = verts;
	ViewPoint * dst = shifted;
	for (int i = 0; i < n; i++, src++, dst++)
	{
		dst->x = src->x - point.x;
		dst->y = src->y - point.y;
	}

	int num_crossings = 0;

	for (i = 0; i < n; i++)
	{
		int i1 = (i == 0) ? i + n - 1 : i - 1;

		ViewPoint * p1 = shifted + i;
		ViewPoint * p2 = shifted + i1;

		if (((p1->y > 0) && (p2->y <= 0)) ||
			((p2->y > 0) && (p1->y <= 0)))
		{
		// Compute intersection.
			float x = (p1->x * p2->y - p2->x * p1->y) / (p2->y - p1->y);

			if (x > 0.0)
			{
				num_crossings++;
			}
		}
	}

	return (num_crossings & 1);
}

//

void Camera::setFOV( SINGLE fovAngle )
{
    camera->set_Vertical_FOV(fovAngle);
    camera->set_Vertical_to_horizontal_aspect(3.0 / 4.0);

}

//

SINGLE Camera::get_far_plane_distance(void)
{
    return camera->zfar;
}

//

void Camera::set_far_plane_distance(SINGLE new_z)
{
    camera->zfar = new_z;
}