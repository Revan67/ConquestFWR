#include "PCH.h"
#include "Misc.h"

#include "BaseCam.h"
#include "CodeMsg.h"
#include "DABaseCamera.h"
#include "Matrix.h"
#include "MatrixUtil.h"
// --------------------------------------------------------------------------
BaseCamera* CreateBaseCamera(unsigned int displayWidth, unsigned int displayHeight)
{
	ViewRect p;
	p.x0 = p.y0 = 0;
	p.x1 = displayWidth - 1;
	p.y1 = displayHeight - 1;

	BaseCamera* cam = new BaseCamera(ENG, &p);
	ASSERT(cam);

#if 0
	float CameraFarClippingPlane = 1e4;
	float HorizontalFov = 45.0f;
	float HVAspect = 4.0f/3.0f;

	PROPERTY camera_properties[] =
	{
		DP_SINGLE ("Near plane distance", 1.00f),
		DP_SINGLE ("Far plane distance", CameraFarClippingPlane),
		DP_SINGLE ("Horizontal FOV", HorizontalFov),
		DP_SINGLE ("Horizontal to vertical aspect", HVAspect),
		DP (NULL, 0) 
	};
	cam->SetProperties(camera_properties);
#else
	float CameraFarClippingPlane = 5000;
	float HorizontalFov = 35.0f;
	float HVAspect = 4.0f/3.0f;

	cam->set_near_plane_distance(.1);
	cam->set_far_plane_distance(CameraFarClippingPlane);
	cam->set_Horizontal_FOV(2 * HorizontalFov);
	cam->set_Horizontal_to_vertical_aspect(HVAspect);
#endif

	Matrix o;
	o.set_identity ();
	const float DEFAULT_START_DISTANCE = 40.0f;

	cam->set_orientation(-1, o);
	cam->set_position(-1, o.get_k () * DEFAULT_START_DISTANCE);

	return cam;
}
// --------------------------------------------------------------------------
DXDEF_ROS const ROS::DABaseCamera* CameraCreate(unsigned int displayWidth, unsigned int displayHeight)
{
	return GetBaseCam(CreateBaseCamera(displayWidth, displayHeight));
}
// --------------------------------------------------------------------------
DXDEF_ROS Transform __cdecl CameraGetTransform(const ROS::DABaseCamera* camera)
{
	return ENG->get_transform(GetBaseCamera(camera)->index);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraSetTransform(const ROS::DABaseCamera* camera, const Transform& transform)
{
	ENG->set_transform(GetBaseCamera(camera)->index, transform);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CameraScale(const ROS::DABaseCamera* camera, float scale)
{
	BaseCamera* cam = GetBaseCamera(camera);

	Transform	trans = cam->get_transform();

	trans.scale(scale);

	cam->set_transform(trans);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraSetPosition(const ROS::DABaseCamera* camera, const Vector& position)
{
	GetBaseCamera(camera)->set_position(position);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraGetPosition(const ROS::DABaseCamera* camera, Vector& position)
{
	position = GetBaseCamera(camera)->get_position();
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraSetOrientation(const ROS::DABaseCamera* camera, const ROS::Matrix& orientation)
{
	Matrix orient(orientation.GetI(), orientation.GetJ(), orientation.GetK());
		
	GetBaseCamera(camera)->set_orientation(orient);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraGetOrientation(const ROS::DABaseCamera* camera, ROS::Matrix& orientation)
{
	Matrix orient = GetBaseCamera(camera)->get_orientation();

	orientation.SetI(orient.get_i());
	orientation.SetJ(orient.get_j());
	orientation.SetK(orient.get_k());
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraDestroy(const ROS::DABaseCamera* camera)
{
	delete GetBaseCamera(camera);
}
// --------------------------------------------------------------------------
DXDEF float __cdecl CameraGetVerticalFOV(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera)->get_fovy();
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraSetVerticalFOV(const ROS::DABaseCamera* camera, float vFOV)
{
	GetBaseCamera(camera)->set_Vertical_FOV(2 * vFOV);
}
// --------------------------------------------------------------------------
DXDEF_ROS float __cdecl CameraGetHorizontalFOV(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera)->get_fovx();
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraSetHorizontalFOV(const ROS::DABaseCamera* camera, float hFOV)
{
	GetBaseCamera(camera)->set_Horizontal_FOV(2 * hFOV);
}
// --------------------------------------------------------------------------
DXDEF_ROS float __cdecl CameraGetAspectRatio(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera)->get_aspect();
}
// --------------------------------------------------------------------------
DXDEF_ROS float __cdecl CameraGetZNear(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera)->get_znear();
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CameraSetZNear(const ROS::DABaseCamera* camera, float zNear)
{
	GetBaseCamera(camera)->set_near_plane_distance(zNear);
}
// --------------------------------------------------------------------------
DXDEF_ROS float __cdecl CameraGetZFar(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera)->get_zfar();
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CameraSetZFar(const ROS::DABaseCamera* camera, float zFar)
{
	GetBaseCamera(camera)->set_far_plane_distance(zFar);
}
// --------------------------------------------------------------------------
DXDEF void __cdecl CameraMoveBy(const ROS::DABaseCamera* camera, float dx, float dy, float dz)
{
	Transform xform = ENG->get_transform(GetBaseCamera(camera)->index);
	xform.move_position(dx, dy, dz);
	ENG->set_transform(GetBaseCamera(camera)->index, xform);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraGetScreenToPoint(const ROS::DABaseCamera* camera, int screenX, int screenY, Vector& worldPoint)
{
	BaseCamera* cam = GetBaseCamera(camera);

	cam->screen_to_point(worldPoint, cam->get_transform(), screenX, screenY);
}
// --------------------------------------------------------------------------
DXDEF_ROS void __cdecl CameraGetPointToScreen(const ROS::DABaseCamera* camera, Vector& worldPoint, int &screenX, int &screenY, float &depth)
{
	BaseCamera* cam = GetBaseCamera(camera);

	float sx, sy;
	if (cam->point_to_screen(sx, sy, depth, cam->get_transform(), worldPoint))
	{
		screenX = sx;
		screenY = sy;
	}
	else
	{
		// The point is behind the camera.
		depth = 0;
	}
}
// --------------------------------------------------------------------------
DXDEF_ROS BaseCamera* CameraGetBaseCamera(const ROS::DABaseCamera* camera)
{
	return GetBaseCamera(camera);
}
// --------------------------------------------------------------------------

