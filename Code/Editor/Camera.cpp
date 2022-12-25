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

 
#include "stdafx.h"
#include "globals.h"

#include "Camera.h"
#include "SuperTrans.h"
#include "Startup.h"
#include "DCamera.h"
#include "CQTrace.h"
#include "SystemStructs.h"
#include "Campaign.h"
#include "Scenario.h"
#include "Resource.h"

#include <system.h>
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define USE_PIPE
#if defined(USE_PIPE)
	#define GFX PIPE
#else
	#define GFX BATCH
#endif

#define HITHER (-1.0F)

#define MOVIEMAXHEIGHT IDEAL2REALY(100)
#define DEFAULT_CAMPITCH -40
#define DEFAULT_MINZ   30000
#define DEFAULT_MAXZ   200000
//#define DEFAULT_MAXZ  ((CQFLAGS.bExtCameraZoom)?200000:120000)

#define SCROLL_X  100
#define SCROLL_Y  100
#define SCROLL_SLOP 4000

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

// static char szRegKey[] = "Camera";

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE Camera : public IBaseCamera, BaseCamera, IEventCallback
{
	BEGIN_DACOM_MAP_INBOUND(Camera)
		DACOM_INTERFACE_ENTRY(ICamera)
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

	TRANSFORM cam2World, worldROT, inverseWorldROT, inverseTransform, inverseSectorTransform, sectorTransform;
	Vector orbitCenter, orbitPosition;	// world coordinate of point around which the camera orbits during rotation
	SINGLE orbitRotation;  //world rotation when orbit started

	PANE * ppane, pane;
	
	SINGLE pitch, roll, yaw;
	SINGLE minZ, maxZ;

//	COMPTR<IViewer> viewer;
//	COMPTR<IDocument> doc;
	CAMERA_DATA data;

	BOOL32 bIgnoreUpdate;
	U32 eventHandle;
	BOOL32 bHasFocus;

	U32 zoomRemainder;				// fractionall part of scrolling not taken
	U32 zoomInTime, zoomOutTime;    // frame_time() >> 10

	// scrolling vars
	U32    bSingleStep;
	U32    scrollRemainderX, scrollRemainderY;  // fractionall part of scrolling not taken
	Vector p[4];					            // rotated world coordinates on viewable area
	BOOL32 bCameraDataChanged;
	S32    lastFrameTime;				// frame period, in microseconds

	//------------------------

	Camera (void);

	~Camera (void);

	/* IBaseCamera methods */

	DEFMETHOD_(struct _pane *,GetPane) (void) const;

	DEFMETHOD_(BOOL32,SetPane) (struct _pane *pane, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetPaneRef) (struct _pane *pane, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetInterfaceBarHeight) (U32 height);

	DEFMETHOD_(const class Transform *,GetTransform) (void) const;

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
	
	DEFMETHOD(SetModelView) (const class Transform *object_to_world = 0) const;

	DEFMETHOD_(BOOL32,SetLookAtPosition) (const Vector &position);

	DEFMETHOD_(Vector,GetLookAtPosition) (void) const;

	DEFMETHOD_(BOOL32,SnapToTargetRotation) (void);

	DEFMETHOD_(SINGLE,GetCameraLOD) (void);

	virtual bool SphereInFrustrum(const Vector &pos,float radius_3d,float & cx,float & cy,float & radius_2d,float & depth);

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

	void handleToggleZoom (void);

	void updateRotationKeys (U32 hotkey, U32 dt);

	void resetCamera (BOOL32 bUpdate);

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

	void scrollUp();
	void scrollDown();
	void scrollLeft();
	void scrollRight();
};
//--------------------------------------------------------------------------//
//
Camera::Camera (void) : BaseCamera(ENGINE, 0)
{
	bHasFocus = TRUE;
	ppane = &pane;
	zoomInTime = 0;
	zoomOutTime = 0;
	rotMode = ROT_NONE;
	scrollRemainderY=0;
	scrollRemainderX=0;
}
//--------------------------------------------------------------------------//
//
Camera::~Camera (void)
{
	if (SYSTEM)
	{
		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Unadvise(eventHandle);
		}
	}
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
	return 1;
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

	ISector* sector = NULL;
	if (CAMPAIGN->GetCurrentScenario() )
	{
		sector = CAMPAIGN->GetCurrentScenario()->GetActiveSector();
	}

	if( sector )
	{
		RECT rect;

		sector->GetSystemRect(sector->GetCurrentSystem(),&rect,false);
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

	if( sector )
	{
		S32 x, y;
		sector->GetSectorCenter(&x, &y);
		pos.x = x;
		pos.y = y;
		pos.z = 0;
		negpos = -pos;
		trans1.set_position(negpos);
		if (CQFLAGS.bSectormapRotates)
			trans2.rotate_about_k(rotation * MUL_DEG_TO_RAD);
		trans2 = trans2.multiply(trans1);
		trans1.set_position(pos);
		sectorTransform = trans1.multiply(trans2);
		inverseSectorTransform = sectorTransform.get_inverse();
	}
		
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

	ISector* sector = NULL;
	if (CAMPAIGN->GetCurrentScenario() )
	{
		sector = CAMPAIGN->GetCurrentScenario()->GetActiveSector();
	}

	if (sector)
	{
		RECT rect;
		
		sector->GetSystemRect(sector->GetCurrentSystem(),&rect,false);
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

	if( sector )
	{
		S32 x, y;
		sector->GetSectorCenter(&x, &y);
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
	}
		
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
			SINGLE d;
			diff.zero();

			ISector* sector = NULL;
			if (CAMPAIGN->GetCurrentScenario() )
			{
				sector = CAMPAIGN->GetCurrentScenario()->GetActiveSector();
			}

			RECT rect = { 0, 165887, 165887, 0};
			if( sector )
				sector->GetSystemRect(sector->GetCurrentSystem(),&rect,false);

			PaneToPoints(p0, p1, p2, p3);

			p[0] = p0;
			p[1] = p1;
			p[2] = p2;
			p[3] = p3;

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

	POINT pt; GetCursorPos( &pt ); x = pt.x; y = pt.y;

	if (x>=pane.x0 && x<=pane.x1 && y>=pane.y0 && y<=pane.y1)	// if cursor with camera pane
	{
		screen_to_point(k, x, y);
		// k is screen position at znear
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
					break;

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
							break;
					}
					break; // end case IDH_ROTATE_WORLD_RIGHT
				} // end switch ((U32)param)
		} // end if (bHasFocus)
	break; // end case CQE_HOTKEY
	
	case WM_COMMAND:
		break;

	case WM_MOUSEWHEEL:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			onMouseWheel(short(HIWORD(msg->wParam)));
		}
		break;

	case CQE_UPDATE:
		if (bHasFocus && CQFLAGS.bGameActive)
		{
			lastFrameTime = U32(param) >> 10;

			// for rotation of camera
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_LEFT))
				updateRotationKeys(IDH_ROTATE_WORLD_LEFT, lastFrameTime);
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_RIGHT))
				updateRotationKeys(IDH_ROTATE_WORLD_RIGHT, lastFrameTime);
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_UP))
				updateRotationKeys(IDH_ROTATE_WORLD_UP, lastFrameTime);
			if (HOTKEY->GetHotkeyState(IDH_ROTATE_WORLD_DOWN))
				updateRotationKeys(IDH_ROTATE_WORLD_DOWN, lastFrameTime);

			// for scrolling of camea
			if (HOTKEY->GetHotkeyState(IDH_SCROLL_DOWNLEFT))
			{
				scrollDown();
				scrollLeft();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_DOWNRIGHT))
			{
				scrollDown();
				scrollRight();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_UPLEFT))
			{
				scrollUp();
				scrollLeft();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_UPRIGHT))
			{
				scrollUp();
				scrollRight();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_DOWN))
			{
				scrollDown();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_LEFT))
			{
				scrollLeft();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_RIGHT))
			{
				scrollRight();
			}
			else if (HOTKEY->GetHotkeyState(IDH_SCROLL_UP))
			{
				scrollUp();
			}

			// zooming
			updateZoom(lastFrameTime);
		}
		updateRotation(lastFrameTime);
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
void Camera::resetCamera (BOOL32 bUpdate)
{
	data.lookAt.zero();
	
	data.version       = CAMERA_DATA_VERSION;
	data.rotateRate    = 90; // degrees per second
	data.zoomRate      = 100000;
	data.toggleZoomZ   = 0;
	data.worldRotation = 0;
	data.FOV_x         = 8.0;
	data.FOV_y         = 0.0;
	data.position.x    =  20000.0f; //  20,000
	data.position.y    = -90000.0f; // -90,000 
	data.position.z    =  90000.0f; //  90,000
	data.pitch         = DEFAULT_CAMPITCH; 
	data.minZ          = DEFAULT_MINZ;
	data.maxZ          = DEFAULT_MAXZ;
	data.worldRotation = 0;

	SetStateInfo(&data, bUpdate);
}
//--------------------------------------------------------------------------//
//
void Camera::SetCameraDefaults (struct CAMERA_DATA & cameraData) const
{
	//sector uses this to stabilize the camera to the most recent defaults

	cameraData.FOV_x = 8.0;
	cameraData.FOV_y = 0.0;
	cameraData.pitch = DEFAULT_CAMPITCH; 
	cameraData.minZ = DEFAULT_MINZ;
	cameraData.maxZ = DEFAULT_MAXZ;
	cameraData.pitch = DEFAULT_CAMPITCH;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::CreateViewer (void)
{
	return 0;
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
	//
	// get top right corner
	//
	p1.x = ppane->x1;
	p1.y = ppane->y0;
	p1.z = 0;
	ScreenToPoint(p1.x, p1.y, 0, 0);
	//
	// get bottom right corner
	//
	p2.x = ppane->x1;
	p2.y = ppane->y1;
	p2.z = 0;
	ScreenToPoint(p2.x, p2.y, 0, 0);
	//
	// get bottom left corner
	//
	p3.x = ppane->x0;
	p3.y = ppane->y1;
	p3.z = 0;
	ScreenToPoint(p3.x, p3.y, 0, 0);
	
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
	
//	DWORD dwWritten;
//	if (doc)
//	{
//		doc->SetFilePointer(0,0);
//		doc->WriteFile(0, &data, sizeof(data), &dwWritten, 0);
//		doc->UpdateAllClients(0);
//	}
//	else
		SetStateInfo(&data, 0);

	bIgnoreUpdate--;

    if (this == CAMERA)
		EVENTSYS->Send(CQE_CAMERA_MOVED);			// tell everyone that something is different about camera

	return 1;
}
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
	data = *cameraData;

	SetHorizontalFOV(data.FOV_x, 0);

	SetWorldRotation(data.worldRotation, 0);

	transform.translation = data.position;
	minZ = data.minZ;
	maxZ = data.maxZ;

	Vector newpos = inverseWorldROT.rotate_translate(transform.translation);

	cam2World.set_position(newpos);

	SetOrientation(data.pitch, 0, 0, update);

	return 1;
}

static BOOL32 bIdentityView=0;
static bNullPane = false;

//--------------------------------------------------------------------------//
//
GENRESULT Camera::SetPerspective ()
{

	bNullPane = false;

	VIEWRECT rect = ppane;
	set_pane(&rect);
	SetHorizontalFOV(data.FOV_x, 0);
	GFX->set_viewport(ppane->x0,ppane->y0,ppane->x1-ppane->x0+1,ppane->y1-ppane->y0+1);
	GFX->set_perspective(fovy, aspect, znear, zfar);
	
	return GR_OK;
}

GENRESULT Camera::SetModelView (const class Transform *object_to_world) const
{

	Transform to_view = inverseTransform;
	//
	// If object-to-world transform supplied, concatenate world-to-view 
	// and object-to-world transforms to get object-to-view transform
	//
	if (object_to_world != NULL)
	{
		to_view = to_view.multiply(*object_to_world);
	}

	GFX->set_modelview(to_view);

	bIdentityView = FALSE;

	return GR_OK;
}


//--------------------------------------------------------------------------
//
void OrthoView (const PANE *pane)
{
	Transform trans;
	trans.translation.x = -0.5f;
	trans.translation.y = -0.5f;
	GFX->set_modelview(trans);
	
	if (pane == 0 && bNullPane == false)
	{
		bNullPane = true;
		GFX->set_ortho(0,SCREENRESX,SCREENRESY,0,-1,+1);
		GFX->set_viewport(0,0,SCREENRESX,SCREENRESY);
	}
	else if (pane != 0)
	{
		bNullPane = false;
		GFX->set_ortho(pane->x0,pane->x1+1,pane->y1+1,pane->y0,-1,1);// left, right, bottom, top, near, far
		GFX->set_viewport(pane->x0,pane->y0,pane->x1-pane->x0+1,pane->y1-pane->y0+1);
	}
	
	GFX->set_render_state( D3DRS_CULLMODE, D3DCULL_NONE);
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
//--------------------------------------------------------------------------//
//
void Camera::scrollUp ()
{
	Vector pos = CAMERA->GetRotatedPosition();

	if (bSingleStep)
	{
		pos.y += SCROLL_Y;
		scrollRemainderY=scrollRemainderX=0;
	}
	else
	{
		U32 scrollRate = CQVARS.scrollRate * fabs(p[2].y - p[1].y);
		U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderY;
		
		pos.y += value >> 10;
		scrollRemainderY = value & 1023;
	}
	CAMERA->SetRotatedPosition(&pos, 1);
}
//--------------------------------------------------------------------------//
//
void Camera::scrollDown ()
{
	Vector pos = CAMERA->GetRotatedPosition();

	if (bSingleStep)
	{
		pos.y -= SCROLL_Y;
		scrollRemainderY=scrollRemainderX=0;
	}
	else
	{
		U32 scrollRate = CQVARS.scrollRate * fabs(p[2].y - p[1].y);
		U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderY;
		
		pos.y -= value >> 10;
		scrollRemainderY = value & 1023;
	}
	CAMERA->SetRotatedPosition(&pos, 1);
}
//--------------------------------------------------------------------------//
//
void Camera::scrollLeft ()
{
	Vector pos = CAMERA->GetRotatedPosition();

	if (bSingleStep)
	{
		pos.x -= SCROLL_X;
		scrollRemainderY=scrollRemainderX=0;
	}
	else
	{
		U32 scrollRate = CQVARS.scrollRate * fabs(p[1].x - p[0].x);
		U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderX;
		
		pos.x -= value >> 10;
		scrollRemainderX = value & 1023;
	}
	CAMERA->SetRotatedPosition(&pos, 1);
}
//--------------------------------------------------------------------------//
//
void Camera::scrollRight ()
{
	Vector pos = CAMERA->GetRotatedPosition();

	if (bSingleStep)
	{
		pos.x += SCROLL_X;
		scrollRemainderY=scrollRemainderX=0;
	}
	else
	{
		U32 scrollRate = CQVARS.scrollRate * fabs(p[1].x - p[0].x);
		U32 value = ((U32(lastFrameTime) >> 10) * scrollRate) + scrollRemainderX;
		
		pos.x += value >> 10;
		scrollRemainderX = value & 1023;
	}
	CAMERA->SetRotatedPosition(&pos, 1);
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
	camera->resetCamera(0);

	return camera;
}

struct _camera : GlobalComponent
{
	Camera * camera;

	virtual void Startup (void)
	{
		Vector pos;
		pos.x = pos.y = 0; 
		pos.z = 9000;

		PANE pane;
		pane.window = NULL;
		pane.x0     = 0;
		pane.y0     = 0;
		pane.x1     = SCREEN_WIDTH-1;
		pane.y1     = SCREEN_HEIGHT-1;

		CAMERA_INIT info;
		memset(&info, 0, sizeof(info));

		info.flags        = CIF_MENUID | CIF_HFOV | CIF_POS | CIF_PITCH | CIF_ROLL | CIF_YAW | CIF_PANE;
		info.pos          = &pos;
		info.pitch        = -40;
		info.roll         = 0;
		info.yaw          = 0;
		info.hfov         = 8;
		info.viewerMenuID = 0;
		info.pane         = &pane;

		CAMERA = camera = CreateCamera(&info);
		CAMERALIB = camera;
		CAMERA->AddRef();
		AddToGlobalCleanupList((IDAComponent **) &CAMERA);
		AddToGlobalCleanupList((IDAComponent **) &CAMERALIB);
	}

	virtual void Initialize (void)
	{
		camera->resetCamera(0);
		camera->SetLookAtPosition( Vector(0,0,0) );

		COMPTR<IDAConnectionPoint> connection;
		if (SYSTEM->QueryOutgoingInterface("IEventCallback", connection) == GR_OK)
		{
			connection->Advise(camera->getBase(), &camera->eventHandle);
		}
	}
};
static _camera camera;

//--------------------------------------------------------------------------//
//-----------------------------End Camera.cpp-------------------------------//
//--------------------------------------------------------------------------//

