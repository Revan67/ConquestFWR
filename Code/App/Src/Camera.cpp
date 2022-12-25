//--------------------------------------------------------------------------//
//                                                                          //
//                                Camera.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Jasony $

   $Header: /Conquest/App/Src/Camera.cpp 84    11/06/00 3:41p Jasony $

*/
//--------------------------------------------------------------------------//

 
#include "pch.h"
#include <globals.h>

#include "Camera.h"
#include "TDocClient.h"
#include "Menu.h"
#include "SuperTrans.h"
#include "UserDefaults.h"
#include "Hotkeys.h"
#include "DBHotkeys.h"
#include "Sector.h"
#include "EventPriority.h"
#include "BaseHotRect.h"
#include "lightman.h"
#include "Startup.h"
#include "Resource.h"
#include "DCamera.h"
#include "VideoSurface.h"
#include "CQBatch.h"

#include <BaseCam.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <Engine.h>
#include <3DMath.h>
#include <VFX.h>
#include <Viewer.h>
#include <ViewCnst.h>
#include <Document.h>
#include <IDocClient.h>
#include <EventSys.h>
#include <HKEvent.h>
#include <MemFile.h>
#include <IRenderPrimitive.h>
#include <WindowManager.h>

#define HITHER (-1.0F)

#define MOVIEMAXHEIGHT IDEAL2REALY(100)
#define DEFAULT_CAMPITCH -40
#define DEFAULT_MINZ   30000
#define DEFAULT_MAXZ   ((CQFLAGS.bExtCameraZoom)?200000:120000)

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
static char szRegKey[] = "Camera";
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE Camera : public IBaseCamera, BaseCamera, IEventCallback, DocumentClient

{
	BEGIN_DACOM_MAP_INBOUND(Camera)
	DACOM_INTERFACE_ENTRY(ICamera)
	DACOM_INTERFACE_ENTRY(IDocumentClient)
	DACOM_INTERFACE_ENTRY_REF("IViewer", viewer)
	DACOM_INTERFACE_ENTRY(IEventCallback)
	END_DACOM_MAP()


	enum CAMERA_ROTATION
	{
		ROT_NONE,
		ROT_RIGHT,	
		ROT_LEFT,
		ROT_UP,
		ROT_DOWN
	} rotMode;

	struct VIEWRECT : ViewRect
	{
		VIEWRECT (const _pane * pane)
		{
			x0 = pane->x0;
			x1 = pane->x1;
			y0 = pane->y0;
			y1 = pane->y1;
		}
	};

	SINGLE targetRotation;
	U32 rotateRemainder;			// fractional part of scrolling not taken
	
	//
	// worldROT -- transform from rotated view to world coordinates
	// inverseWorldROT  -- transform from world coordinates to rotated world coordinates
	//

	TRANSFORM modelTransform, cam2World, worldROT, inverseWorldROT, inverseTransform, inverseSectorTransform, sectorTransform;
	
	Vector orbitCenter, orbitPosition;	// world coordinate of point around which the camera orbits during rotation
	SINGLE orbitRotation;  //world rotation when orbit started

	SINGLE shakeTimeLeft;
	SINGLE shakeTimeTotal;
	SINGLE shakeNoise;
	Vector noiseVector;

	PANE * ppane, pane;
	
	SINGLE pitch, roll, yaw;
	SINGLE minZ, maxZ;

	COMPTR<IViewer> viewer;
	COMPTR<IDocument> doc;
	CAMERA_DATA data;

	BOOL32 bIgnoreUpdate;
	U32 menuID;
	U32 eventHandle;
	BOOL32 bHasFocus;

	U32 zoomRemainder;				// fractionall part of scrolling not taken
	U32 zoomInTime, zoomOutTime;    // frame_time() >> 10

	U32 interface_bar_height;

	bool bFrustumOld;
	float frustum[6][4];

//	S32 movieModeHeight;
	
	//------------------------

	Camera (void);

	~Camera (void);

    void * operator new (size_t size)
	{
		return calloc(size, 1);
	}

	void   operator delete (void *ptr)
	{
		::free(ptr);
	}

	/* IBaseCamera methods */

	DEFMETHOD_(struct _pane *,GetPane) (void) const;

	DEFMETHOD_(BOOL32,SetPane) (struct _pane *pane, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetPaneRef) (struct _pane *pane, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetInterfaceBarHeight) (U32 height);

	DEFMETHOD_(const class Transform *,GetTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetModelTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetInverseTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetInverseWorldTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetWorldTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetInverseSectorTransform) (void) const;  /* world to rotated_sector transform */

	DEFMETHOD_(const class Transform *,GetSectorTransform) (void) const;  /* rotated sector to world transform */

	DEFMETHOD_(SINGLE,GetHorizontalFOV) (void) const;

	DEFMETHOD_(BOOL32,SetHorizontalFOV) (SINGLE fx, BOOL32 update=1);

	DEFMETHOD_(SINGLE,GetVerticalFOV) (void) const;

	DEFMETHOD_(BOOL32,SetVerticalFOV) (SINGLE fy, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetOrbitPosition) (void);

	DEFMETHOD_(BOOL32,SetWorldRotation) (SINGLE rotation, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetWorldRotationPitchRoll) (SINGLE rotation, SINGLE pitch, SINGLE roll, BOOL32 update=1);

	DEFMETHOD_(SINGLE,GetWorldRotation) (void) const;

	DEFMETHOD_(void,GetOrientation) (SINGLE * pitch, SINGLE * roll, SINGLE * yaw) const;

	DEFMETHOD_(BOOL32,SetOrientation) (SINGLE pitch, SINGLE roll, SINGLE yaw, BOOL32 update=1);

	DEFMETHOD_(class Vector,GetPosition) (void) const;

	DEFMETHOD_(BOOL32,SetPosition) (const class Vector * newPos, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetRotatedPosition) (const class Vector * newPos, BOOL32 update=1);

	DEFMETHOD_(class Vector,GetRotatedPosition) (void) const;

	DEFMETHOD_(BOOL32,MoveForward) (SINGLE distance, BOOL32 update=1);

	DEFMETHOD_(BOOL32,AddToPitch) (SINGLE delta, BOOL32 update=1);

	DEFMETHOD_(TRANSRESULT,PointToScreen) (const Vector &point, S32 *pane_X, S32 *pane_Y, const Transform *object_to_world) const;

	DEFMETHOD_(BOOL32,PaneToPoints) (Vector & top, Vector & bottom, Vector & left, Vector & right) const;

	DEFMETHOD_(BOOL32,ScreenToPoint) (SINGLE & x, SINGLE & y, SINGLE z) const;

	DEFMETHOD_(U32,GetStateInfo) (struct CAMERA_DATA * cameraData) const;

	DEFMETHOD_(BOOL32,SetStateInfo) (const struct CAMERA_DATA * cameraData, BOOL32 update=1);

	DEFMETHOD(SetPerspective) ();
	
	DEFMETHOD(SetModelView) (const class Transform *object_to_world = 0);

	DEFMETHOD_(BOOL32,SetLookAtPosition) (const Vector &position);

	DEFMETHOD_(Vector,GetLookAtPosition) (void) const;

	DEFMETHOD_(BOOL32,SnapToTargetRotation) (void);

	DEFMETHOD_(SINGLE,GetCameraLOD) (void);

	virtual bool SphereInFrustrum(const Vector &pos,float radius_3d,float & cx,float & cy,float & radius_2d,float & depth);

	virtual bool SphereInFrustrumFast(const Vector &pos,float radius_3d);

	virtual void ShakeCamera(SINGLE durration, SINGLE power);

	DEFMETHOD_(void,SetCameraDefaults) (struct CAMERA_DATA & cameraData) const;

	DEFMETHOD_(void,SetCameraToDefaults) (void)
	{
		resetCamera(1);
	}

	/* IEventCallback methods */

	DEFMETHOD(Notify) (U32 message, void *param = 0);

	/* IDocumentClient methods */

	DEFMETHOD(OnUpdate) (struct IDocument *doc, const C8 *message = 0, void *parm = 0);

	/* ICamera methods */

	virtual const Vector & COMAPI get_look_pos(void) const;

	virtual Transform COMAPI get_inverse_transform (void)
	{
		return inverseTransform;
	}
	
	/* Camera methods */

	void OnNoOwner (void)
	{
	}

	BOOL32 CreateViewer (void);

	BOOL32 UpdateViewer (void);

	BOOL32 ScreenToPoint (SINGLE & x, SINGLE & y, SINGLE z, BOOL32 bTransform) const;

	void updateZoom (U32 dt);

	void onMouseWheel (S32 zDelta);

	void updateRotation (U32 dt);

	void updateShake(SINGLE dt);

//	void updateMovieMode(U32 dt);

	void handleToggleZoom (void);

	void updateRotationKeys (U32 hotkey, U32 dt);

	void resetCamera (BOOL32 bUpdate);

	void getFrustum();

	BOOL32 setVerticalRotation(SINGLE rotation, BOOL32 update=1);

	//assumes a square rect., left=bottom=0
	inline bool inCircle(S32 x, S32 y, const RECT &rect)
	{
		S32 width = (rect.right)/2;
		width = width*width;
		S32 xVal = ((rect.right)/2)-x;
		S32 yVal = ((rect.top)/2)-y;
		S32 dist = xVal*xVal+yVal*yVal;
		return dist < width;
	}
	
	IDAComponent * getBase (void)
	{
		return static_cast<ICamera *> (this);
	}
};
//--------------------------------------------------------------------------//
//
Camera::Camera (void) : BaseCamera(ENGINE, 0)
{
	shakeTimeLeft = 0;
	shakeTimeTotal = 0;
	shakeNoise = 0;
	bHasFocus = TRUE;
	ppane = &pane;
}
//--------------------------------------------------------------------------//
//
Camera::~Camera (void)
{
	if (FULLSCREEN)
	{
		COMPTR<IDAConnectionPoint> connection;
		
		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Unadvise(eventHandle);
	}

#ifdef _DEBUG
	DEFAULTS->SetDataInRegistry(szRegKey, &data, sizeof(data));
#endif
}
//--------------------------------------------------------------------------//
//
struct _pane * Camera::GetPane (void) const
{
	return ppane;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetPane (struct _pane *newPane, BOOL32 update)
{
	pane = *newPane;
	VIEWRECT rect = &pane;
	set_pane(&rect);
	ppane = &pane;
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetPaneRef (struct _pane *newPane, BOOL32 update)
{
	VIEWRECT rect = newPane;
	set_pane(&rect);
	ppane = newPane;
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetInterfaceBarHeight (U32 height)
{
	interface_bar_height = height;
	
//	if (CQFLAGS.bMovieMode == 0)
//	{
		ppane->x1 = SCREENRESX - 1;
		ppane->y1 = SCREENRESY - height - 1;

		VIEWRECT rect = ppane;
		set_pane(&rect);
		SetStateInfo(&data, 1);
//	}

	return 1;
}

//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetModelTransform (void) const
{
	return &modelTransform;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetTransform (void) const
{
	return &transform;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetInverseTransform (void) const
{
	return &inverseTransform;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetInverseWorldTransform (void) const
{
	return &inverseWorldROT;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetWorldTransform (void) const
{
	return &worldROT;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetInverseSectorTransform (void) const
{
	return &inverseSectorTransform;
}
//--------------------------------------------------------------------------//
//
const class Transform * Camera::GetSectorTransform (void) const
{
	return &sectorTransform;
}
//--------------------------------------------------------------------------//
//
SINGLE Camera::GetHorizontalFOV (void) const
{
	return fovx;		// half angle
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetHorizontalFOV (SINGLE fx, BOOL32 update)
{
	set_Horizontal_FOV(fx);

//	float _aspect = float(SCREEN_WIDTH) / float(SCREEN_HEIGHT);  //we don't know why
	float _aspect = float(pane.x1 + 1 - pane.x0) / float(pane.y1 + 1 - pane.y0);	// width / height

	set_Horizontal_to_vertical_aspect(_aspect);

	if (update)
		UpdateViewer();
	return 1;
}
//--------------------------------------------------------------------------//
//
SINGLE Camera::GetVerticalFOV (void) const
{
	return fovx;	// get half angle
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetVerticalFOV (SINGLE fy, BOOL32 update)
{
	set_Vertical_FOV(fy);

//	float _aspect = (float)(pane.y1 + 1 - pane.y0) / (float)(pane.x1 + 1 - pane.x0);	// height / width
	float _aspect = 1.0F;

	set_Vertical_to_horizontal_aspect(_aspect);
	
	if (update)
		UpdateViewer();
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetOrbitPosition (void)
{
	Vector pos, k, center;
	SINGLE t;

	//
	// find the center position
	//

	k = cam2World.get_k();	// get neg look vector
	pos = cam2World.translation;

	//
	// z = pos.z + t * k.z
	// z-pos.z = t * k.z 
	// (z-pos.z) / k.z = t
	// x = pos.x + t * k.x
	// y = pos.y + t * k.y
	//

	if (k.z == 0.0)
		k.z = 0.1;

	t = (-pos.z) / k.z;
	
	center.x = pos.x + (t * k.x);
	center.y = pos.y + (t * k.y);
	center.z = 0;
	orbitCenter = worldROT.rotate_translate(center);
	orbitPosition = transform.translation;
	orbitRotation = data.worldRotation;

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetWorldRotation (SINGLE rotation, BOOL32 update)
{
	TRANSFORM trans1, trans2;
	Vector pos, negpos;

	while (rotation > 180)
		rotation -= 360;
	while (rotation < -180)
		rotation += 360;

	data.worldRotation = rotation;

	if (SECTOR)
	{
		RECT rect;
		
		SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(),&rect);
		pos.x = (rect.left + rect.right) / 2;
		pos.y = (rect.top + rect.bottom) / 2;
		pos.z = 0;
		negpos = -pos;
	}
	else
	{
		memset(&pos, 0, sizeof(pos));
		memset(&negpos, 0, sizeof(negpos));
	}

	trans1.set_position(negpos);
	trans2.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans2 = trans2.multiply(trans1);
	trans1.set_position(pos);
	worldROT = trans1.multiply(trans2);
	inverseWorldROT = worldROT.get_inverse();

	//
	// now orbit the camera around the orbit point
	//
	if (update)
	{
		trans2.set_identity();
		trans2.rotate_about_k((rotation-orbitRotation) * MUL_DEG_TO_RAD);

		pos = orbitPosition;
		pos -= orbitCenter;
		pos = trans2.rotate(pos);
		pos += orbitCenter;
		transform.translation = pos;

		pos = inverseWorldROT.rotate_translate(transform.translation);		// get rotated position
		cam2World.set_position(pos);
	}

	//
	// now calculate the sector rotation transform
	//
	trans1.set_identity();
	trans2.set_identity();

	S32 x, y;

	SECTOR->GetSectorCenter(&x, &y);
	pos.x = x;
	pos.y = y;
	pos.z = 0;
	negpos = -pos;
	trans1.set_position(negpos);
	if (DEFAULTS->GetDefaults()->bSectormapRotates)
		trans2.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans2 = trans2.multiply(trans1);
	trans1.set_position(pos);
	sectorTransform = trans1.multiply(trans2);
	inverseSectorTransform = sectorTransform.get_inverse();
		
	return SetOrientation(pitch, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::setVerticalRotation(SINGLE rotation, BOOL32 update)
{
	pitch = rotation;
	Vector pos = orbitCenter;
	SINGLE dist = (orbitCenter-orbitPosition).fast_magnitude();
	SetOrientation(pitch, roll, yaw, update);
	SetLookAtPosition(pos);
	SetOrbitPosition();
	SINGLE newDist =  (orbitCenter-orbitPosition).fast_magnitude();
	SINGLE distance =newDist-dist;

	Vector newpos;
	
	newpos = transform.translation;

	Vector k;

	k = -transform.get_k();

	k *= distance;
	
	newpos += k;

	transform.set_position(newpos);
	cam2World.set_position(inverseWorldROT.rotate_translate(newpos));	// save pos in rotated coordinates
	data.toggleZoomZ = 0;

	return SetOrientation(pitch, roll, yaw, update);

/*	TRANSFORM trans = transform;
	Vector k = -(trans.get_k());
	SINGLE dist = (orbitCenter-orbitPosition).fast_magnitude();

	trans.translation += k*dist;
	trans.rotate_about_i(pitch-rotation);
	k = -(trans.get_k());
	trans.translation -= k*dist;

	transform.set_position(trans.translation);
	cam2World.set_position(inverseWorldROT.rotate_translate(transform.translation));

	return SetOrientation(trans.get_pitch(), roll, yaw, update);*/
}

//--------------------------------------------------------------------------//
//
SINGLE Camera::GetWorldRotation (void) const
{
	return data.worldRotation;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetWorldRotationPitchRoll (SINGLE rotation, SINGLE pitch, SINGLE roll, BOOL32 update)
{
	TRANSFORM trans1, trans2;
	Vector pos, negpos;

	while (rotation > 180)
		rotation -= 360;
	while (rotation < -180)
		rotation += 360;

	data.worldRotation = rotation;

	if (SECTOR)
	{
		RECT rect;
		
		SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(),&rect);
		pos.x = (rect.left + rect.right) / 2;
		pos.y = (rect.top + rect.bottom) / 2;
		pos.z = 0;
		negpos = -pos;
	}
	else
	{
		memset(&pos, 0, sizeof(pos));
		memset(&negpos, 0, sizeof(negpos));
	}

	trans1.set_position(negpos);
	trans2.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans2 = trans2.multiply(trans1);
	trans1.set_position(pos);
	worldROT = trans1.multiply(trans2);
	inverseWorldROT = worldROT.get_inverse();

	//
	// now orbit the camera around the orbit point
	//
	if (update)
	{
		trans2.set_identity();
		trans2.rotate_about_k((rotation-orbitRotation) * MUL_DEG_TO_RAD);

		pos = orbitPosition;
		pos -= orbitCenter;
		pos = trans2.rotate(pos);
		pos += orbitCenter;
		transform.translation = pos;

		pos = inverseWorldROT.rotate_translate(transform.translation);		// get rotated position
		cam2World.set_position(pos);
	}

	//
	// now calculate the sector rotation transform
	//
	trans1.set_identity();
	trans2.set_identity();

	S32 x, y;

	SECTOR->GetSectorCenter(&x, &y);
	pos.x = x;
	pos.y = y;
	pos.z = 0;
	negpos = -pos;
	trans1.set_position(negpos);
	trans2.rotate_about_k(rotation * MUL_DEG_TO_RAD);
	trans2 = trans2.multiply(trans1);
	trans1.set_position(pos);
	sectorTransform = trans1.multiply(trans2);
	inverseSectorTransform = sectorTransform.get_inverse();
		
	return SetOrientation(pitch, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
void Camera::GetOrientation (SINGLE * _pitch, SINGLE * _roll, SINGLE * _yaw) const
{
	if (_pitch)
		*_pitch = pitch;
	if (_roll)
		*_roll = roll;
	if (_yaw)
		*_yaw = yaw;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetOrientation (SINGLE _pitch, SINGLE _roll, SINGLE _yaw, BOOL32 update)
{
	Vector i, j, k;

	cam2World.set_orientation(_pitch, _roll, _yaw);

	pitch = _pitch;
	roll = _roll;
	yaw = _yaw;

	transform = worldROT.multiply(cam2World);
	transform.translation += noiseVector;
	inverseTransform = transform.get_inverse();

	{
		Vector current;

		current.x = (ppane->x1 + ppane->x0 + 1) / 2;
		current.y = (1*(ppane->y1 - ppane->y0 + 1) / 2) + ppane->y0;
		current.z = 0;
	
		ScreenToPoint(current.x, current.y, 0, 1);
		
		data.lookAt = current;

	//
	// get top left corner
	//
	// get top right corner
	//
	// get bottom right corner
	//
	// get bottom left corner

		// make sure look-at position is within the system boudary
		if (update)
		{
			bool bMovedRight=false;
			bool bMovedDown=false;
			bool bMovedLeft=false;
			bool bMovedUp=false;
			Vector p0, p1, p2, p3, diff;
			SINGLE xBound, yMid, disc;
			RECT rect;
			SINGLE d;
			SECTOR->GetSystemRect(SECTOR->GetCurrentSystem(),&rect);
			diff.zero();

			PaneToPoints(p0, p1, p2, p3);

			if ((d = p0.y - rect.top - HALFGRID) > 0)
			{
				bMovedDown = true;
				diff.y -= d;
			}
			if (bMovedDown==false && (d = p2.y - rect.bottom + HALFGRID) < 0)
			{
				bMovedUp = true;
				diff.y -= d;
			}
			// xBound = Ymid +/- sqrt(r^2 - (Y-Ymid)^2)

			yMid = (rect.right / 2) + HALFGRID;
			disc = (yMid*yMid) - (( ((p0.y+p2.y+diff.y+diff.y)*0.5) - yMid ) * ( ((p0.y+p2.y+diff.y+diff.y)*0.5) - yMid ));
			
			if (disc > 0)
			{
				disc = sqrt(disc);
				disc += HALFGRID;
				xBound = yMid - disc;

				if ((d = p0.x - xBound) < 0)
				{
					bMovedRight = true;
					diff.x -= d;
				}
				xBound = yMid + disc;

				if (bMovedRight==false && (d = p1.x - xBound) > 0)
				{
					bMovedLeft = true;
					diff.x -= d;
				}

				if (bMovedRight||bMovedDown||bMovedLeft||bMovedUp)
				{
					cam2World.translation += diff;
					SetOrientation(_pitch, _roll, _yaw, 0);
				}
			}
		}
	}

	if (update)
	{
		UpdateViewer();
	}

	return 1;
}
//--------------------------------------------------------------------------//
//
class Vector Camera::GetPosition (void) const
{
	return transform.translation;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetPosition (const class Vector * newPos, BOOL32 update)
{
	Vector rotated;

	rotated = inverseWorldROT.rotate_translate(*newPos);

	cam2World.set_position(rotated);
	transform.translation = *newPos;

	return SetOrientation(pitch, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetRotatedPosition (const class Vector * newPos, BOOL32 update)
{
	cam2World.set_position(*newPos);
	transform.translation = worldROT.rotate_translate(*newPos);

	return SetOrientation(pitch, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
class Vector Camera::GetRotatedPosition (void) const
{
	return cam2World.get_position();
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::MoveForward (SINGLE distance, BOOL32 update)
{
	Vector newpos;
	
	newpos = transform.translation;

#if 0
	Vector k = -transform.get_k();
#else
	S32 x, y;
	Vector k;

	WM->GetCursorPos(x, y);
	if (x>=pane.x0 && x<=pane.x1 && y>=pane.y0 && y<=pane.y1)	// if cursor with camera pane
	{
		screen_to_point(k, x, y);
		//k is screen position at znear
		k *= (distance/znear);
	}
	else
	{
		k = -transform.get_k();
		k *= distance;
	}
#endif

	if (distance < 0 && newpos.z + k.z > maxZ)
	{
		if (newpos.z < maxZ)
		{
			SINGLE z = newpos.z - maxZ;		// max travel distance (negative since distance is negative)
			k *= (z / distance);
		}
		else
			return 0;
	}
	else
	if (distance > 0 && newpos.z + k.z < minZ)
	{
		if (newpos.z > minZ)
		{
			SINGLE z = newpos.z - minZ;		// max travel distance
			k *= (z / distance);
		}
		else
			return 0;
	}
	
	newpos += k;
	transform.set_position(newpos);
	cam2World.set_position(inverseWorldROT.rotate_translate(newpos));	// save pos in rotated coordinates
	data.toggleZoomZ = 0;

	return SetOrientation(pitch, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::AddToPitch (SINGLE delta, BOOL32 update)
{
	pitch += delta;

	return SetOrientation(pitch + delta, roll, yaw, update);
}
//--------------------------------------------------------------------------//
//
static S32 get_closest_90_degree (S32 angle)
{
	S32 result = angle - (angle % 90);

	if ((result - angle) < -45)
		result += 90;
	else
	if ((result - angle) > 45)
		result -= 90;

	return result;
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
GENRESULT Camera::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case CQE_KILL_FOCUS:
		bHasFocus = 0;
		break;
	case CQE_SET_FOCUS:
		bHasFocus = 1;
		break;
	case WM_CLOSE:
		if (viewer)
			viewer->set_display_state(0);
		break;
	
	case CQE_HOTKEY:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			switch ((U32)param)
			{                   
			case IDH_TOGGLE_ZOOM:
				handleToggleZoom();
				break;
			case IDH_ROTATE_0_WORLD:
				targetRotation = 0;
				if (data.worldRotation < 0)
					rotMode = ROT_LEFT;
				else
				if (data.worldRotation > 0)
					rotMode = ROT_RIGHT;
				SetOrbitPosition();
				break; // end case IDH_ROTATE_0_WORLD
			case IDH_ROTATE_90_WORLD_LEFT:
				switch (rotMode)
				{
				case ROT_NONE:
					{
						S32 diff = get_closest_90_degree(data.worldRotation) - S32(data.worldRotation);
						if (diff > 180)
							diff -= 360;
						else
						if (diff < -180)
							diff += 360;
						if (diff > 0)
							targetRotation = S32(data.worldRotation) + diff;
						else
						{
							if ((targetRotation = data.worldRotation + 90) > 180)
								targetRotation -= 360;
							targetRotation = get_closest_90_degree(targetRotation);
						}

						SetOrbitPosition();
						rotMode = ROT_LEFT;
					}
					break;

				case ROT_RIGHT:
					if ((targetRotation = targetRotation + 90) > 180)
						targetRotation -= 360;
					targetRotation = get_closest_90_degree(targetRotation);

					rotMode = ROT_LEFT;
					// spinTime = ELAPSED_TIME*2.5;
					break;
				}
				break;  // end case IDH_ROTATE_WORLD_LEFT

			case IDH_ROTATE_90_WORLD_RIGHT:
				switch (rotMode)
				{
				case ROT_NONE:
					{
						S32 diff = get_closest_90_degree(data.worldRotation) - S32(data.worldRotation);
						if (diff > 180)
							diff -= 360;
						else
						if (diff < -180)
							diff += 360;
						if (diff < 0)
							targetRotation = S32(data.worldRotation) + diff;
						else
						{
							if ((targetRotation = data.worldRotation - 90) < -180)
								targetRotation += 360;
							targetRotation = get_closest_90_degree(targetRotation);
						}

						SetOrbitPosition();
						rotMode = ROT_RIGHT;
					}
					break;

				case ROT_LEFT:
					if ((targetRotation = targetRotation - 90) < -180)
						targetRotation += 360;
					targetRotation = get_closest_90_degree(targetRotation);

					rotMode = ROT_RIGHT;
					// spinTime = ELAPSED_TIME*(-1.5);
					break;
				}
				break; // end case IDH_ROTATE_WORLD_RIGHT
			} // end switch ((U32)param)
		} // end if (bHasFocus)
		break; // end case CQE_HOTKEY
	
	case WM_COMMAND:
		if (LOWORD(msg->wParam) == menuID)
		{
			if (viewer)
				viewer->set_display_state(1);
		}
		else
		if (LOWORD(msg->wParam) == IDM_RESET_CAMERA)
			resetCamera(1);
		break;

	case WM_MOUSEWHEEL:
		if (bHasFocus && CQFLAGS.bGameActive)
			onMouseWheel(short(HIWORD(msg->wParam)));
		break;

	case CQE_UPDATE:
		updateShake((U32(param)>>10)/1000.0f);
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_LEFT))
				updateRotationKeys(IDH_ROTATE_WORLD_LEFT, U32(param) >> 10);
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_RIGHT))
				updateRotationKeys(IDH_ROTATE_WORLD_RIGHT, U32(param) >> 10);
			if (DBHOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_UP))
				updateRotationKeys(IDH_ROTATE_WORLD_UP, U32(param) >> 10);
			if (DBHOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_DOWN))
				updateRotationKeys(IDH_ROTATE_WORLD_DOWN, U32(param) >> 10);
			updateZoom(U32(param) >> 10);
		}
		updateRotation(U32(param) >> 10);
//		updateMovieMode(U32(param) >> 10);
		break;
	}

	return GR_OK;
}
//----------------------------------------------------------------------//
//
void Camera::updateRotationKeys (U32 hotkey, U32 dt)
{
	switch (hotkey)
	{
	case IDH_ROTATE_WORLD_LEFT:
		if ((targetRotation = data.worldRotation + SINGLE(data.rotateRate*dt)/1000) > 180)
			targetRotation -= 360;
		SetOrbitPosition();
		rotMode = ROT_LEFT;
		break;  // end case IDH_ROTATE_WORLD_LEFT

	case IDH_ROTATE_WORLD_RIGHT:
		if ((targetRotation = data.worldRotation - SINGLE(data.rotateRate*dt)/1000) < -180)
			targetRotation += 360;
		SetOrbitPosition();
		rotMode = ROT_RIGHT;
		break; // end case IDH_ROTATE_WORLD_RIGHT

	case IDH_ROTATE_WORLD_UP:
		if ((targetRotation = data.pitch - SINGLE(data.rotateRate*dt)/10000) < -180)
			targetRotation += 360;
		SetOrbitPosition();
		rotMode = ROT_UP;
		break; // end case IDH_ROTATE_WORLD_UP

	case IDH_ROTATE_WORLD_DOWN:
		if ((targetRotation = data.pitch + SINGLE(data.rotateRate*dt)/10000) < 180)
			targetRotation -= 360;
		SetOrbitPosition();
		rotMode = ROT_DOWN;
		break; // end case IDH_ROTATE_WORLD_UP
	} // end switch (hotkey)
}
//----------------------------------------------------------------------//
//
GENRESULT Camera::OnUpdate (struct IDocument *doc, const C8 *message, void *parm)
{
	DWORD dwRead;

	if (bIgnoreUpdate == 0)
	{
		doc->SetFilePointer(0,0);
		doc->ReadFile(0, &data, sizeof(data), &dwRead, 0);

		SetStateInfo(&data, 1);
	}
	

	return GR_OK;
}
//--------------------------------------------------------------------------//
//
const Vector & COMAPI Camera::get_look_pos(void) const
{
	return data.lookAt;
}

//--------------------------------------------------------------------------//
//
void Camera::resetCamera (BOOL32 bUpdate)
{
	data.worldRotation = 0;

	data.FOV_x = 8.0;
	data.FOV_y = 0.0;
	data.position.x = 20000; data.position.y = -90000; data.position.z = 90000.0;
	data.pitch = DEFAULT_CAMPITCH; 
	data.minZ = DEFAULT_MINZ;
	data.maxZ = DEFAULT_MAXZ;

	SetStateInfo(&data, bUpdate);
}
//----------------------------------------------------------------------//
//
void Camera::getFrustum()
{
	bFrustumOld = false;
	Matrix4 proj;
	PIPE->get_projection(proj);
	Transform modView;
	PIPE->get_modelview(modView);
	Matrix4 modView4(modView);

	Matrix4 clip;
	clip = modView*proj;
	clip.transpose();

	SINGLE t;
	/* Extract the numbers for the RIGHT plane */
	frustum[0][0] = clip.d[0][ 3] - clip.d[0][ 0];
	frustum[0][1] = clip.d[1][ 3] - clip.d[1][ 0];
	frustum[0][2] = clip.d[2][3] - clip.d[2][ 0];
	frustum[0][3] = clip.d[3][3] - clip.d[3][0];

	/* Normalize the result */
	t = Sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
	frustum[0][0] /= t;
	frustum[0][1] /= t;
	frustum[0][2] /= t;
	frustum[0][3] /= t;

	/* Extract the numbers for the LEFT plane */
	frustum[1][0] = clip.d[0][ 3] + clip.d[0][ 0];
	frustum[1][1] = clip.d[1][ 3] + clip.d[1][ 0];
	frustum[1][2] = clip.d[2][ 3] + clip.d[2][ 0];
	frustum[1][3] = clip.d[3][ 3] + clip.d[3][ 0];

	/* Normalize the result */
	t = Sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
	frustum[1][0] /= t;
	frustum[1][1] /= t;
	frustum[1][2] /= t;
	frustum[1][3] /= t;

	/* Extract the BOTTOM plane */
	frustum[2][0] = clip.d[0][ 3] + clip.d[0][ 1];
	frustum[2][1] = clip.d[1][ 3] + clip.d[1][ 1];
	frustum[2][2] = clip.d[2][ 3] + clip.d[2][ 1];
	frustum[2][3] = clip.d[3][ 3] + clip.d[3][ 1];

	/* Normalize the result */
	t = Sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
	frustum[2][0] /= t;
	frustum[2][1] /= t;
	frustum[2][2] /= t;
	frustum[2][3] /= t;

	/* Extract the TOP plane */
	frustum[3][0] = clip.d[0][ 3] - clip.d[0][ 1];
	frustum[3][1] = clip.d[1][ 3] - clip.d[1][ 1];
	frustum[3][2] = clip.d[2][ 3] - clip.d[2][ 1];
	frustum[3][3] = clip.d[3][ 3] - clip.d[3][ 1];

	/* Normalize the result */
	t = Sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
	frustum[3][0] /= t;
	frustum[3][1] /= t;
	frustum[3][2] /= t;
	frustum[3][3] /= t;

	/* Extract the FAR plane */
	frustum[4][0] = clip.d[0][ 3] - clip.d[0][ 2];
	frustum[4][1] = clip.d[1][ 3] - clip.d[1][ 2];
	frustum[4][2] = clip.d[2][ 3] - clip.d[2][ 2];
	frustum[4][3] = clip.d[3][ 3] - clip.d[3][ 2];

	/* Normalize the result */
	t = Sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
	frustum[4][0] /= t;
	frustum[4][1] /= t;
	frustum[4][2] /= t;
	frustum[4][3] /= t;

	/* Extract the NEAR plane */
	frustum[5][0] = clip.d[0][ 3] + clip.d[0][ 2];
	frustum[5][1] = clip.d[1][ 3] + clip.d[1][ 2];
	frustum[5][2] = clip.d[2][ 3] + clip.d[2][ 2];
	frustum[5][3] = clip.d[3][ 3] + clip.d[3][ 2];

	/* Normalize the result */
	t = Sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
	frustum[5][0] /= t;
	frustum[5][1] /= t;
	frustum[5][2] /= t;
	frustum[5][3] /= t;
}
//--------------------------------------------------------------------------//
//
void Camera::SetCameraDefaults (struct CAMERA_DATA & cameraData) const
{
	//sector uses this to stabilize the camera to the most recent defaults
//	cameraData.worldRotation = 0;

	cameraData.FOV_x = 8.0;
	cameraData.FOV_y = 0.0;
//	cameraData.position.x = 20000; cameraData.position.y = -90000; cameraData.position.z = 90000.0;
	cameraData.pitch = DEFAULT_CAMPITCH; 
	cameraData.minZ = DEFAULT_MINZ;
	cameraData.maxZ = DEFAULT_MAXZ;

	cameraData.minZ = DEFAULT_MINZ;
	cameraData.maxZ = DEFAULT_MAXZ;
	cameraData.pitch = DEFAULT_CAMPITCH;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::CreateViewer (void)
{
	DOCDESC ddesc = "CameraData";
	COMPTR<IFileSystem> pMemFile;

	if (DEFAULTS->GetDataFromRegistry(szRegKey, &data, sizeof(data)) != sizeof(data) || data.version != CAMERA_DATA_VERSION)
	{
		resetCamera(0);

		data.version = CAMERA_DATA_VERSION;
		data.rotateRate = 90;    // degrees per second
		data.zoomRate = 100000;
		data.toggleZoomZ = 0;
	}

	if (CQFLAGS.bNoGDI)
		return 1;


	// create a memory file
	MEMFILEDESC mdesc = ddesc.lpFileName;
	mdesc.dwDesiredAccess  = GENERIC_WRITE | GENERIC_READ;
	mdesc.lpBuffer = &data;
	mdesc.dwBufferSize = sizeof(data);
	mdesc.dwFlags = CMF_DONT_COPY_MEMORY;
	if (DACOM->CreateInstance(&mdesc, pMemFile) != GR_OK)
		return 0;

	pMemFile->AddRef();
	ddesc.lpParent = pMemFile;
	ddesc.dwDesiredAccess  = GENERIC_WRITE | GENERIC_READ;
	ddesc.dwCreationDistribution = CREATE_ALWAYS;
	ddesc.lpImplementation = "DOS";

	if (DACOM->CreateInstance(&ddesc, doc) == GR_OK)
	{
		VIEWDESC vdesc;
		HWND hwnd;

		vdesc.className = "CAMERA_DATA";
		vdesc.doc = doc;
		vdesc.hOwnerWindow = hMainWindow;
		
		if (PARSER->CreateInstance(&vdesc, viewer) == GR_OK)
		{
			COMPTR<IDAConnectionPoint> connection;

			viewer->get_main_window((void **) &hwnd);
			MoveWindow(hwnd, 100, 100, 400, 200, 1);
			viewer->set_instance_name("Main camera");

			MakeConnection(doc);
		}
	}

	return (viewer != 0);
}
//--------------------------------------------------------------------------//
//
TRANSRESULT Camera::PointToScreen (const Vector &point, S32 *pane_X, S32 *pane_Y, const Transform *object_to_world) const
{
	//
	// Obtain world-to-view transform by inverting the view-to-world transform
	// (i.e., the camera's position and orientation in world space)
	//
	
	Transform to_view = inverseTransform;
	
	//
	// If object-to-world transform supplied, concatenate world-to-view 
	// and object-to-world transforms to get object-to-view transform
	//
	
	if (object_to_world != NULL)
	{
		to_view = to_view.multiply(*object_to_world);
	}
	
	//
	// Transform point from world or object space to viewspace
	//
	
	Vector view = to_view.rotate_translate(point);
	
	//
	// Return if point lies behind the front clipping plane
	//
	TRANSRESULT result = IN_PANE;
	
	if (view.z >= HITHER)
	{
		result = BEHIND_CAMERA;

		view.z = 2*HITHER-view.z;
	}

	//
	// Project point from view space to perspective space
	//
	
	DOUBLE w = -1.0 / DOUBLE(view.z);
	DOUBLE x = view.x * w;
	DOUBLE y = view.y * w;
	
	//
	// See if point lies outside perspective-space frustum
	//
	
/*	
	if ((x >  x_clip) ||
		(x < -x_clip) ||
		(y >  y_clip) ||
		(y < -y_clip))
	{
		result = OUT_PANE;
	}
*/	
	//
	// Normalize point to output pane coordinates and return
	//
	
	//basecam definition of hpc and vpc is now optimal!
	if (pane_X != NULL)
	{
		*pane_X = (S32) ((x * hpc) + x_screen_center + 0.5F);
	}
	
	if (pane_Y != NULL)
	{
		*pane_Y = (S32) ((y * vpc) + y_screen_center + 0.5F);
	}
	
	return result;
}
//--------------------------------------------------------------------------//
//  returns ROTATED WORLD COORDINATES, meaning map coordinates
//  meaning p0.x is the smallest x
//
BOOL32 Camera::PaneToPoints (Vector & p0, Vector & p1, Vector & p2, Vector & p3) const
{
	//
	// get top left corner
	//
	p0.x = ppane->x0;
	p0.y = ppane->y0;
	p0.z = 0;
	ScreenToPoint(p0.x, p0.y, 0, 0);
//	p0 = inverseWorldROT.rotate_translate(p0);
	//
	// get top right corner
	//
	p1.x = ppane->x1;
	p1.y = ppane->y0;
	p1.z = 0;
	ScreenToPoint(p1.x, p1.y, 0, 0);
//	p1 = inverseWorldROT.rotate_translate(p1);
	//
	// get bottom right corner
	//
	p2.x = ppane->x1;
	p2.y = ppane->y1;
	p2.z = 0;
	ScreenToPoint(p2.x, p2.y, 0, 0);
//	p2 = inverseWorldROT.rotate_translate(p2);
	//
	// get bottom left corner
	//
	p3.x = ppane->x0;
	p3.y = ppane->y1;
	p3.z = 0;
	ScreenToPoint(p3.x, p3.y, 0, 0);
//	p3 = inverseWorldROT.rotate_translate(p3);
	
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::ScreenToPoint (SINGLE & x, SINGLE & y, SINGLE z) const
{
	return ScreenToPoint(x, y, z, 1);
}
//--------------------------------------------------------------------------//
//  returns ROTATED world coordinates if bTransform is FALSE
//  returns ABSOLUTE world coordinates if bTransform is TRUE
//
BOOL32 Camera::ScreenToPoint (SINGLE & x, SINGLE & y, SINGLE z, BOOL32 bTransform) const
{
	Vector k, result, pos=transform.translation;         // cam2World.get_position();
	SINGLE t;

	screen_to_point(k, x, y);

	//
	// z = pos.z + t * k.z
	// z-pos.z = t * k.z 
	// (z-pos.z) / k.z = t
	// x = pos.x + t * k.x
	// y = pos.y + t * k.y
	//

	if (k.z == 0.0)
		return 0;

	t = (z-pos.z) / k.z;
	
	result.x = pos.x + (t * k.x);
	result.y = pos.y + (t * k.y);
	result.z = 0;
 	if (bTransform==0)
		result = inverseWorldROT.rotate_translate(result);		// get rotated position

	x = result.x;
	y = result.y;

	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::UpdateViewer (void)
{
	DWORD dwWritten;

	bIgnoreUpdate++;

	data.FOV_x = fovx*2.0;
	data.FOV_y = fovy*2.0;
	data.position = transform.translation;
	data.pitch = pitch;

	if (pitch+fovy < 0)
	{
		SINGLE dist = transform.translation.z/sin((-pitch+fovy)*PI/180.0);
		set_near_plane_distance(max(dist-12000.0,100.0));
		dist = transform.translation.z/sin((-pitch-fovy)*PI/180.0);
		set_far_plane_distance(dist*1.3);
	}
	else
	{
		set_near_plane_distance(100.0);
		set_far_plane_distance(20000.0);
	}
	
	if (doc)
	{
		doc->SetFilePointer(0,0);
		doc->WriteFile(0, &data, sizeof(data), &dwWritten, 0);
		doc->UpdateAllClients(0);
	}
	else
		SetStateInfo(&data, 0);

	bIgnoreUpdate--;

    if (this == CAMERA)
		EVENTSYS->Send(CQE_CAMERA_MOVED);			// tell everyone that something is different about camera

	return 1;
}
//--------------------------------------------------------------------------//
//
/*
void Camera::SetMyFOV (void)
{
	if ((fovX = horiz_FOV) == 0)
	{
		fovX = atan(x_screen_center/hpc);
		fovX *= 2.0 / MUL_DEG_TO_RAD;
	}

	if ((fovY = vert_FOV) == 0)
	{
		fovY = atan(-y_screen_center / vpc);
		fovY *= 2.0 / MUL_DEG_TO_RAD;
	}
}
*/
//--------------------------------------------------------------------------//
//
U32 Camera::GetStateInfo (struct CAMERA_DATA * cameraData) const
{
	*cameraData = data;
	return sizeof(data);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetStateInfo (const struct CAMERA_DATA * cameraData, BOOL32 update)
{
	if (cameraData->version != CAMERA_DATA_VERSION)
	{
		CQTRACE12("Ignoring CAMERA_DATA with old version number=%d (Should be %d)", cameraData->version, CAMERA_DATA_VERSION);
	}
	else
		data = *cameraData;

	SetHorizontalFOV(data.FOV_x, 0);
//	SetVerticalFOV(data.FOV_y, 0);

	SetWorldRotation(data.worldRotation, 0);

	transform.translation = data.position;
	minZ = data.minZ;
	maxZ = data.maxZ;

	Vector newpos = inverseWorldROT.rotate_translate(transform.translation);

	cam2World.set_position(newpos);

	SetOrientation(data.pitch, 0, 0, update);

	return 1;
}
//--------------------------------------------------------------------------//
//
#if 0
static void Transform_to_4x4 (float m[16], const Transform &t)
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
#endif

static BOOL32 bPerspective=0;
static BOOL32 bNullView=0;
static BOOL32 bIdentityView=0;
static bool bNullPane = false;
//--------------------------------------------------------------------------//
//
GENRESULT Camera::SetPerspective ()
{
	bNullPane = false;
//	if (!bPerspective)
//	{
/*		if(movieModeHeight)
		{
			ppane->x0 = 0;
			ppane->y0 = movieModeHeight;
			ppane->y1 = min(SCREENRESY - interface_bar_height - 1,SCREENRESY - movieModeHeight-1);
			aspect = float(ppane->x1 + 1 - ppane->x0) / float(ppane->y1 + 1 - ppane->y0);	// width / height
			fovy = fovx/aspect;
		}
		else
*///		{
			ppane->x0 = 0;
			//ppane->y0 = 0;
			ppane->y1 = SCREENRESY - interface_bar_height - 1;
//		}

			
		// to guard against NAN floats
		if( !ppane->x0 && !ppane->x1 )
		{
			ppane->x1 = SCREENRESX - 1;
		}
		if( !ppane->y0 && !ppane->y1 )
		{
			ppane->y1 = SCREENRESY - interface_bar_height - 1;
		}


		VIEWRECT rect = ppane;
		set_pane(&rect);
		SetHorizontalFOV(data.FOV_x, 0);
	//	SetStateInfo(&data, 1);
		BATCH->set_viewport(ppane->x0,ppane->y0,ppane->x1-ppane->x0+1,ppane->y1-ppane->y0+1);

		//BATCH->set_perspective(fovy, aspect, 100, 5000+transform.translation.z*3.0);	//(jy)
		BATCH->set_perspective(fovy, aspect, znear, zfar);
		bFrustumOld = true;
		
//		bPerspective = TRUE;
//	}

//	bIdentityView = FALSE;

	return GR_OK;
}

GENRESULT Camera::SetModelView (const class Transform *object_to_world)
{
	Transform to_view = inverseTransform;
	
	//
	// If object-to-world transform supplied, concatenate world-to-view 
	// and object-to-world transforms to get object-to-view transform
	//
	if (object_to_world != NULL)
	{
		//modelTransform = *object_to_world;
		to_view = to_view.multiply(*object_to_world);
	}
	else
	{
		//modelTransform.set_identity();
	}
	BATCH->set_modelview(to_view);

	bIdentityView = FALSE;
	bFrustumOld = true;

	return GR_OK;
}


//--------------------------------------------------------------------------
//
void OrthoView (const PANE *pane)
{
	//bPerspective = FALSE;
//	if (!bIdentityView)
//	{
	/*	BATCH->set_modelview(Transform());
		if (CQBATCH)
			CQBATCH->SetIdentityModelview();*/
		Transform trans;
		trans.translation.x = -0.5f;
		trans.translation.y = -0.5f;
		BATCH->set_modelview(trans);
//		bIdentityView = TRUE;
//	}

	if (pane == 0 && bNullPane == false)
	{
		bNullPane = true;
		BATCH->set_ortho(0,SCREENRESX,SCREENRESY,0,-1,+1);//0,MAX_ORTHO_DEPTH);
		BATCH->set_viewport(0,0,SCREENRESX,SCREENRESY);
	}
	else if (pane != 0)
	{
		bNullPane = false;
		BATCH->set_ortho(pane->x0,pane->x1+1,pane->y1+1,pane->y0,-1,1);//0,MAX_ORTHO_DEPTH);		// left, right, bottom, top, near, far
		BATCH->set_viewport(pane->x0,pane->y0,pane->x1-pane->x0+1,pane->y1-pane->y0+1);
	}

	BATCH->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetLookAtPosition (const Vector &newPos)
{
	Vector current, diff;

	current.x = (ppane->x1 + ppane->x0 + 1) / 2;
	current.y = (1*(ppane->y1 - ppane->y0 + 1) / 2) + ppane->y0;
	current.z = 0;

	ScreenToPoint(current.x, current.y, 0, 1);

	diff = newPos - current;
	diff.z = 0;
	diff += GetPosition();

	SetPosition(&diff);

	return 1;
}
//--------------------------------------------------------------------------//
//
Vector Camera::GetLookAtPosition (void) const
{
	return data.lookAt;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SnapToTargetRotation (void)
{
	if (rotMode != ROT_NONE)
		SetWorldRotation(targetRotation);
	return TRUE;
}
//--------------------------------------------------------------------------//
// frameTime is in milliseconds
//
void Camera::updateZoom (U32 frameTime)
{
	if (HOTKEY->GetHotkeyState(IDH_ZOOM_IN))
	{
		zoomInTime += frameTime;
		zoomOutTime = 0;
	}
	if (HOTKEY->GetHotkeyState(IDH_ZOOM_OUT))
	{
		zoomOutTime += frameTime;
		zoomInTime = 0;
	}

	if (zoomInTime)
	{
		U32 value = (frameTime * data.zoomRate) + zoomRemainder;

		zoomRemainder = value & 1023;
		if (S32(zoomInTime -= frameTime) < 0)
			zoomInTime = 0;
			
		CAMERA->MoveForward(value>>10);
	}
	if (zoomOutTime)
	{
		U32 value = (frameTime * data.zoomRate) + zoomRemainder;

		zoomRemainder = value & 1023;
		if (S32(zoomOutTime -= frameTime) < 0)
			zoomOutTime = 0;
	 
		CAMERA->MoveForward(-S32(value>>10));
	}
}
//--------------------------------------------------------------------------//
// -zDelta = rolled toward user, +zDelta = rolled away from user
//
void Camera::onMouseWheel (S32 zDelta)
{
	if (zDelta > 0)
	{
		zoomInTime += 125;
		zoomOutTime = 0;
	}
	else if (zDelta < 0)
	{
		zoomOutTime += 125;
		zoomInTime = 0;
	}
}
//--------------------------------------------------------------------------//
// dt is in milliseconds
//
void Camera::updateRotation (U32 dt)
{
	//
	// world rotation
	//
	if (rotMode == ROT_LEFT)
	{
		SINGLE oldRot, rotation;
		U32 value = (dt * data.rotateRate) + rotateRemainder;
		rotateRemainder = value & 1023;
		
		oldRot = rotation = data.worldRotation;
		rotation += SINGLE(value >> 10);

		S32 diff = targetRotation - oldRot;
		if (diff > 180)
			diff -= 360;
		else
		if (diff < -180)
			diff += 360;

		if (diff < S32(value >> 10))
		{
			rotMode = ROT_NONE;
			CAMERA->SetWorldRotation(targetRotation, 1);
		}
		else
			CAMERA->SetWorldRotation(rotation, 1);
	}
	else
	if (rotMode == ROT_RIGHT)
	{
		SINGLE oldRot, rotation;
		U32 value = (dt * data.rotateRate) + rotateRemainder;
		rotateRemainder = value & 1023;
		 
		oldRot = rotation = data.worldRotation;
		rotation -= SINGLE(value >> 10);

		S32 diff = oldRot - targetRotation;
		if (diff > 180)
			diff -= 360;
		else
		if (diff < -180)
			diff += 360;

		if (diff < S32(value >> 10))
		{
			rotMode = ROT_NONE;
			CAMERA->SetWorldRotation(targetRotation, 1);
		}
		else
			CAMERA->SetWorldRotation(rotation, 1);
	}
	else
	if (rotMode == ROT_UP)
	{
		SINGLE oldRot, rotation;
		U32 value = (dt * data.rotateRate) + rotateRemainder;
		rotateRemainder = value & 1023;
		 
		oldRot = rotation = pitch;
		rotation -= SINGLE(value >> 10);

		S32 diff = oldRot - targetRotation;
		if (diff > 180)
			diff -= 360;
		else
		if (diff < -180)
			diff += 360;

		if (diff < S32(value >> 10))
		{
			rotMode = ROT_NONE;
			setVerticalRotation(targetRotation, 1);
		}
		else
			setVerticalRotation(rotation, 1);
	}
	else
	if (rotMode == ROT_DOWN)
	{
		SINGLE oldRot, rotation;
		U32 value = (dt * data.rotateRate) + rotateRemainder;
		rotateRemainder = value & 1023;
		 
		oldRot = rotation = pitch;
		rotation -= SINGLE(value >> 10);

		S32 diff = oldRot - targetRotation;
		if (diff > 180)
			diff -= 360;
		else
		if (diff < -180)
			diff += 360;

		if (diff < S32(value >> 10))
		{
			rotMode = ROT_NONE;
			setVerticalRotation(targetRotation, 1);
		}
		else
			setVerticalRotation(rotation, 1);
	}
}

void Camera::updateShake(SINGLE dt)
{
	if(dt >= shakeTimeLeft)
	{
		shakeTimeLeft = 0;
		noiseVector = Vector(0,0,0);
	}
	else
	{
		shakeTimeLeft -= dt;
		SINGLE noiseSize = shakeNoise*(shakeTimeLeft/shakeTimeTotal);
		noiseVector = Vector(((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize,
				((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize,
				((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize);
	}
	SetOrientation(pitch, roll, yaw,false);
}

/*
//--------------------------------------------------------------------------//
//
void Camera::updateMovieMode(U32 dt)
{
	if(CQFLAGS.bMovieMode)
	{
		if(movieModeHeight < MOVIEMAXHEIGHT)
		{
			S32 min1 = movieModeHeight+((dt*MOVIEMAXHEIGHT)/1000);
			movieModeHeight = min(MOVIEMAXHEIGHT,min1);
			SetPerspective();
		}
	}
	else if(movieModeHeight)
	{
		movieModeHeight = max(0,(S32)(movieModeHeight-((dt*MOVIEMAXHEIGHT)/1000)));
		SetPerspective();
	}
}
*/
//--------------------------------------------------------------------------//
//
void Camera::handleToggleZoom (void)
{
	if (data.toggleZoomZ == 0)		// not zoomed
	{
		Vector lookat = data.lookAt;
		Vector newpos = cam2World.translation;
		newpos.z = data.maxZ;
		data.toggleZoomZ = cam2World.translation.z;
		SetRotatedPosition(&newpos, 0);
		SetLookAtPosition(lookat);
	}
	else	// already zoomed out
	{
		Vector lookat = data.lookAt;
		Vector newpos = cam2World.translation;
		newpos.z = data.toggleZoomZ;
		data.toggleZoomZ = 0;
		SetRotatedPosition(&newpos, 0);
		SetLookAtPosition(lookat);
	}
}
//--------------------------------------------------------------------------//
//
SINGLE Camera::GetCameraLOD (void)
{
	SINGLE result = 1 - ((transform.translation.z - minZ) / (maxZ - minZ));
	
	if (result < 0)
		result = 0;
	else
	if (result > 1)
		result = 1;

	return result;
}

//-------------------------------------------------------------------
//
bool Camera::SphereInFrustrum(const Vector &pos,float radius_3d,float & cx,float & cy,float & radius_2d,float & depth)
{
	bool result = false;

	Vector vcenter = transform.inverse_rotate_translate(pos);
				
	// Make sure object is in front of near plane.
	if (vcenter.z < -get_znear())
	{
		const struct ViewRect * pane = get_pane();
		
		float x_screen_center = float(pane->x1 - pane->x0) * 0.5f;
		float y_screen_center = float(pane->y1 - pane->y0) * 0.5f;
		float screen_center_x = pane->x0 + x_screen_center;
		float screen_center_y = pane->y0 + y_screen_center;
		
		float w = -1.0 / vcenter.z;
		float sphere_center_x = vcenter.x * w;
		float sphere_center_y = vcenter.y * w;
		
		cx = screen_center_x + sphere_center_x * get_hpc()*get_znear();
		cy = screen_center_y + sphere_center_y * get_vpc()*get_znear();
		
		float center_distance = vcenter.magnitude();
		
		if(center_distance >= radius_3d)
		{
			float dx = fabs(cx - screen_center_x);
			float dy = fabs(cy - screen_center_y);
			
			//changes 1/26 - rmarr
			//function should now not return TRUE with obscene radii
			float outer_angle = asin(radius_3d / center_distance);
			sphere_center_x = fabs(sphere_center_x);
			float inner_angle = atan(sphere_center_x);
			
			//	float near_plane_radius = tan(inner_angle + outer_angle);
			//	near_plane_radius -= sphere_center_x;
			//	radius = near_plane_radius * camera->get_hpc();
			
			float near_plane_radius = tan(inner_angle - outer_angle);
			near_plane_radius = sphere_center_x-near_plane_radius;
			radius_2d = near_plane_radius * get_hpc()*get_znear();
			
			int view_w = (pane->x1 - pane->x0 + 1) >> 1;
			int view_h = (pane->y1 - pane->y0 + 1) >> 1;
			
			if ((dx < (view_w + radius_2d)) && (dy < (view_h + radius_2d)))
			{
				depth = -vcenter.z;
				result = true;
			}
		}
	}
				
	return result;
}
//-------------------------------------------------------------------
//
bool Camera::SphereInFrustrumFast(const Vector &pos,float radius_3d)
{
	//this need to be fixed however the archaic nature of this camera makes it imposible
	SINGLE cx,cy,rad,depth;
	return SphereInFrustrum(pos,radius_3d,cx,cy,rad,depth);
//	if(bFrustumOld)
//		getFrustum();

//	for(U32 i = 0; i < 6; ++i )
//      if( frustum[i][0] * pos.x + frustum[i][1] * pos.y + frustum[i][2] * pos.z + frustum[i][3] <= -radius_3d )
//         return false;
//   return true;

}
//-------------------------------------------------------------------
//
void Camera::ShakeCamera(SINGLE durration, SINGLE power)
{
	shakeTimeLeft = durration;
	shakeTimeTotal = durration;
	shakeNoise = power;
}
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//
//
static Camera * CreateCamera (CAMERA_INIT * info)
{
	Camera * camera;
	Vector position (0,0,1000);

	if ((camera = new DAComponent<Camera>) == 0)
		return 0;

	if (info->flags & CIF_PANE)
	{
		camera->SetPane(info->pane, 0);
	}
	else
	if (info->flags & CIF_PANEREF)
	{
		camera->SetPaneRef(info->pane, 0);
	}

	if (info->flags & CIF_HFOV)
	{
		camera->SetHorizontalFOV(info->hfov,0);
	}
	if (info->flags & CIF_VFOV)
	{
		camera->SetVerticalFOV(info->vfov,0);
	}
	if (info->flags & CIF_POS)
	{
		camera->SetPosition(info->pos, 0);
	}
	else
	{
		camera->SetPosition(&position, 0);	// use default position
	}
	if (info->flags & CIF_ROLL)
	{
		camera->roll = info->roll;
	}
	if (info->flags & CIF_PITCH)
	{
		camera->pitch = info->pitch;
	}
	if (info->flags & CIF_YAW)
	{
		camera->yaw = info->yaw;
	}
	camera->SetOrientation(camera->pitch, camera->roll, camera->yaw, 0);

	return camera;
}

struct _camera : GlobalComponent
{
	Camera * camera;

	virtual void Startup (void)
	{
		CAMERA_INIT info;
		Vector pos;

		memset(&info, 0, sizeof(info));
		pos.x = pos.y = 0;
		pos.z = 9000;
//		pane.x0 = minX;

		info.flags = CIF_MENUID | CIF_HFOV | CIF_POS | CIF_PITCH | CIF_ROLL | CIF_YAW;

		info.pos = &pos;
		info.pitch = -40;
		info.roll = 0;
		info.yaw = 0;
		info.hfov = 8;
		info.viewerMenuID = IDS_VIEWMAINCAMERA;
		CAMERA = camera = CreateCamera(&info);
		MAINCAM = camera;
		MAINCAM->AddRef();
		AddToGlobalCleanupList((IDAComponent **) &MAINCAM);
		AddToGlobalCleanupList((IDAComponent **) &CAMERA);
	}

	virtual void Initialize (void)
	{
		COMPTR<IDAConnectionPoint> connection;

		if (camera->CreateViewer() == 0)
			CQBOMB0("Viewer could not be created.");

//		if (info->flags & CIF_MENUID)
		{
			HMENU hMenu = MENU->GetSubMenu(MENUPOS_VIEW);
			MENUITEMINFO minfo;

			memset(&minfo, 0, sizeof(minfo));
			minfo.cbSize = sizeof(minfo);
			minfo.fMask = MIIM_ID | MIIM_TYPE;
			minfo.fType = MFT_STRING;
			minfo.wID = IDS_VIEWMAINCAMERA;	// info->viewerMenuID;
			minfo.dwTypeData = "Camera";
			minfo.cch = 6;	// length of string "Camera"
			
			if (InsertMenuItem(hMenu, 0x7FFE, 1, &minfo))
				camera->menuID = IDS_VIEWMAINCAMERA;	// info->viewerMenuID;
		}

		if (FULLSCREEN->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
			connection->Advise(camera->getBase(), &camera->eventHandle);

		FULLSCREEN->SetCallbackPriority(camera,EVENT_PRIORITY_CAMERA);
	}
};
static _camera camera;

//--------------------------------------------------------------------------//
//-----------------------------End Camera.cpp-------------------------------//
//--------------------------------------------------------------------------//
