//--------------------------------------------------------------------------//
//                                                                          //
//                                Camera.cpp                                //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /EffectEd/Src/Camera.cpp 4     10/23/03 10:39p Tmauer $

*/
//--------------------------------------------------------------------------//

 
#include "stdafx.h"
#include "globals.h"
#include <DACOM.h>
#include <d3dx9.h>

#include "Camera.h"
#include "SuperTrans.h"
#include "Startup.h"
#include "Resource.h"
#include "IEffectFile.h"
#include "IEffectTarget.h"
#include "PreviewWin.h"

#include <BaseCam.h>
#include <TComponent.h>
#include <TSmartPointer.h>
#include <IConnection.h>
#include <3DMath.h>
#include <RendPipeline.h>

#define HITHER (-1.0F)

#define DEFAULT_CAMPITCH -40
#define DEFAULT_MINZ   20000
#define DEFAULT_MAXZ   55000//((SOMFLAGS.bExtCameraZoom)?200000:120000)
#define DEFAULT_MIN_PITCH -80
#define DEFAULT_MAX_PITCH -30

struct CAMERA_DATA
{
	SINGLE FOV_x, FOV_y;
	Vector position;
	Vector orbitPoint;

	SINGLE zoomRate;		// world units per second
	SINGLE panRate;		// world units per second
	U32 rotateRate;		// degrees per second
	U32 verticalRotateRate;		// degrees per second

	SINGLE maxZoom;
	SINGLE minZoom;
};

//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE Camera : public IBaseCamera, BaseCamera

{
	BEGIN_DACOM_MAP_INBOUND(Camera)
	DACOM_INTERFACE_ENTRY(ICamera)
	END_DACOM_MAP()


	enum CAMERA_ROTATION
	{
		ROT_NONE,
		ROT_RIGHT,	
		ROT_LEFT,
		ROT_UP,
		ROT_DOWN
	} rotMode;

	TRANSFORM inverseTransform;

	Vector orbitPoint;

	ViewRect * ppane, pane;
	
	CAMERA_DATA data;

	CameraMode cameraMode;

	short wheelChange;
	U32 lastTick;
	POINT cursorCenter;

	float frustum[6][4];

	IEffectTarget * hpTarget;
	U32 hpIndex;
	TRANSFORM smoothTransStart;
	Vector lastK;
	SINGLE smoothEndTime;
	SINGLE smoothStartTime;
	bool bSmoothHardpointMode;

	SINGLE nearClipSetting;
	SINGLE farClipSetting;

	SINGLE shakeTimeLeft;
	SINGLE shakeTimeTotal;
	SINGLE shakeNoise;
	Vector trueOrbitPoint;
	
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

	DEFMETHOD_(struct ViewRect *,GetPane) (void) const;

	DEFMETHOD_(BOOL32,SetPane) (struct ViewRect *pane, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetPaneRef) (struct ViewRect *pane, BOOL32 update=1);

	DEFMETHOD_(const class Transform *,GetTransform) (void) const;

	DEFMETHOD_(const class Transform *,GetInverseTransform) (void) const;

	DEFMETHOD_(SINGLE,GetHorizontalFOV) (void) const;

	DEFMETHOD_(BOOL32,SetHorizontalFOV) (SINGLE fx, BOOL32 update=1);

	DEFMETHOD_(SINGLE,GetVerticalFOV) (void) const;

	DEFMETHOD_(BOOL32,SetVerticalFOV) (SINGLE fy, BOOL32 update=1);

	DEFMETHOD_(SINGLE,GetWorldRotation) (void) const;

	DEFMETHOD_(void,GetOrientation) (SINGLE * pitch, SINGLE * roll, SINGLE * yaw) const;

	DEFMETHOD_(BOOL32,SetOrientation) (BOOL32 update=1);

	DEFMETHOD_(class Vector,GetPosition) (void) const;

	DEFMETHOD_(BOOL32,SetPosition) (const class Vector * newPos, BOOL32 update=1);

	DEFMETHOD_(BOOL32,SetRotatedPosition) (const class Vector * newPos, BOOL32 update=1);

	DEFMETHOD_(class Vector,GetRotatedPosition) (void) const;

	DEFMETHOD_(BOOL32,MoveForward) (SINGLE distance, BOOL32 update=1);

	DEFMETHOD_(TRANSRESULT,PointToScreen) (const Vector &point, S32 *pane_X, S32 *pane_Y, const Transform *object_to_world) const;

	DEFMETHOD_(BOOL32,PaneToPoints) (Vector & top, Vector & bottom, Vector & left, Vector & right) const;

	DEFMETHOD_(BOOL32,ScreenToPoint) (SINGLE & x, SINGLE & y, SINGLE z) const;

	DEFMETHOD_(Vector, ScreenToPoint) (SINGLE & x, SINGLE & y);

	DEFMETHOD_(U32,GetStateInfo) (struct CAMERA_DATA * cameraData) const;

	DEFMETHOD_(BOOL32,SetStateInfo) (const struct CAMERA_DATA * cameraData, BOOL32 update=1);

	DEFMETHOD(SetPerspective) ();
	
	GENRESULT SetModelView(const class Transform *object_to_world = 0);

	DEFMETHOD_(BOOL32,SetLookAtPosition) (const Vector &position);

	virtual Vector GetLookAtPosition (void) const;

	DEFMETHOD_(BOOL32,SnapToTargetRotation) (void);

	DEFMETHOD_(SINGLE,GetCameraLOD) (void);

	virtual bool SphereInFrustrum(const Vector &pos,float radius_3d,float & cx,float & cy,float & radius_2d,float & depth);

	virtual bool SphereInFrustrumFast(const Vector &pos,float radius_3d);

	DEFMETHOD_(void,SetCameraDefaults) (struct CAMERA_DATA & cameraData) const;

	DEFMETHOD_(void,SetCameraToDefaults) (void)
	{
		resetCamera(1);
	}

	virtual void Update();

	virtual void PlaneScroll(SINGLE vert,SINGLE horz);

	virtual void PanCamera(SINGLE horz,SINGLE vert);

	virtual void OrbitCamera(SINGLE yawChange, SINGLE pitchChange);

	virtual void ZoomCamera(SINGLE zoom);

	virtual enum CameraMode GetCameraMode()
	{
		return cameraMode;
	}

	virtual void ResetCamera(BOOL32 updates);

	virtual void RecenterCamera(Vector newCenter);

	virtual U32 Notify(U32 message, void *param = 0);

	virtual void SetHardpointMode(IEffectTarget * target,U32 hpIndex);

	virtual void SetHardpointModeSmooth(IEffectTarget * target,U32 hpIndex, SINGLE gameTimeEnd);

	virtual void EndHardpointMode();

	virtual void EndHardpointModeSmooth(SINGLE gameTimeEnd);

	virtual void ResetClipPlane();
	
	virtual void SetClipPlaneNear(SINGLE set);

	virtual void SetClipPlaneFar(SINGLE set);

	virtual void CameraShake(SINGLE durration, SINGLE power);

	/* ICamera methods */

	virtual Transform COMAPI get_inverse_transform (void)
	{
		return inverseTransform;
	}

	virtual const Vector & COMAPI get_look_pos(void) const 
	{
		return orbitPoint;
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

	void resetCamera (BOOL32 bUpdate);

	void getFrustum();

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
Camera::Camera (void) : BaseCamera(ENGINE,0)
{
	ppane = &pane;
	cameraMode = CAMERA_NOMODE;
	lastTick = 0;
	wheelChange = 0;
	orbitPoint = Vector(0,0,0);
	trueOrbitPoint = Vector(0,0,0);
	hpTarget = NULL;

	shakeTimeLeft = 0;
	shakeTimeTotal = 0;
	shakeNoise = 0;
}
//--------------------------------------------------------------------------//
//
Camera::~Camera (void)
{
}
//--------------------------------------------------------------------------//
//
struct ViewRect * Camera::GetPane (void) const
{
	return ppane;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetPane (struct ViewRect *newPane, BOOL32 update)
{
	pane = *newPane;
	set_pane(&pane);
	ppane = &pane;
	return 1;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetPaneRef (struct ViewRect *newPane, BOOL32 update)
{
	set_pane(newPane);
	ppane = newPane;
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
SINGLE Camera::GetWorldRotation (void) const
{
	return 0;
}
//--------------------------------------------------------------------------//
//
void Camera::GetOrientation (SINGLE * _pitch, SINGLE * _roll, SINGLE * _yaw) const
{
	if (_pitch)
		*_pitch = 0;
	if (_roll)
		*_roll = 0;
	if (_yaw)
		*_yaw = 0;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetOrientation (BOOL32 update)
{
	if(hpTarget)
	{
		bool bNoUpdate = true;
		if(bSmoothHardpointMode)
		{
			SINGLE gameTime = PreviewWin::GetGameTime();
			if(gameTime < smoothEndTime)
			{
				Quaternion q1;
				Quaternion q2;
				TRANSFORM finalTrans;
				hpTarget->GetHardPointTransform(hpIndex,finalTrans);
				q1.set(finalTrans);
				q2.set(smoothTransStart);

				SINGLE t = (gameTime-smoothStartTime)/(smoothEndTime-smoothStartTime);
				transform.translation = (t*(finalTrans.translation-smoothTransStart.translation))+smoothTransStart.translation;

				q1 = slerp(q2,q1,t);
				Matrix m1(q1);

				transform.set_orientation(m1);
				inverseTransform = transform.get_inverse();
	
				bNoUpdate = false;
			}
			else
			{
				bSmoothHardpointMode = false;
			}
		}
		if(bNoUpdate)
		{
			hpTarget->GetHardPointTransform(hpIndex,transform);
			Vector k = transform.get_k();
			k.fast_normalize();
			Vector j = transform.get_j();
			j.fast_normalize();
			Vector i = cross_product(j,k);
			i.fast_normalize();
			j = cross_product(k,i);
			transform.set_i(i);
			transform.set_j(j);
			transform.set_k(k);
			inverseTransform = transform.get_inverse();
		}
	}
	else
	{
		bool bNoUpdate = true;
		if(bSmoothHardpointMode)
		{
			SINGLE gameTime = PreviewWin::GetGameTime();
			if(gameTime < smoothEndTime)
			{
				Quaternion q1;
				Quaternion q2;
				TRANSFORM finalTrans;
				finalTrans.translation = orbitPoint+lastK;
				Vector k = lastK;
				k.fast_normalize();
				Vector j;
				if(k.x != 0 && k.y != 0)
					j = Vector(0,0,1);
				else
					j = Vector(0,1,0);

				Vector i = cross_product(j,k);
				i.fast_normalize();
				j = cross_product(k,i);
				finalTrans.set_i(i);
				finalTrans.set_j(j);
				finalTrans.set_k(k);

				q1.set(finalTrans);
				q2.set(smoothTransStart);

				SINGLE t = (gameTime-smoothStartTime)/(smoothEndTime-smoothStartTime);
				transform.translation = (t*(finalTrans.translation-smoothTransStart.translation))+smoothTransStart.translation;

				q1 = slerp(q2,q1,t);
				Matrix m1(q1);

				transform.set_orientation(m1);
				inverseTransform = transform.get_inverse();

				bNoUpdate = false;
			}
			else
			{
				bSmoothHardpointMode = false;
			}
		}
		if(bNoUpdate)
		{
			Vector k = transform.translation-orbitPoint;
			k.fast_normalize();
			Vector j;
			if(k.x != 0 && k.y != 0)
				j = Vector(0,0,1);
			else
				j = Vector(0,1,0);

			Vector i = cross_product(j,k);
			i.fast_normalize();
			j = cross_product(k,i);
			transform.set_i(i);
			transform.set_j(j);
			transform.set_k(k);

			inverseTransform = transform.get_inverse();
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
	transform.translation = *newPos;

	return SetOrientation(update);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetRotatedPosition (const class Vector * newPos, BOOL32 update)
{
	transform.translation = *newPos;

	return SetOrientation(update);
}
//--------------------------------------------------------------------------//
//
class Vector Camera::GetRotatedPosition (void) const
{
	return transform.translation;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::MoveForward (SINGLE distance, BOOL32 update)
{
	Vector newpos;
	
	newpos = transform.translation;

	Vector k = -transform.get_k();

	k *= distance;

	newpos += k;
	orbitPoint += k;
	trueOrbitPoint += k; 
	transform.set_position(newpos);

	return SetOrientation(update);
}
//--------------------------------------------------------------------------//
// receive notifications from event system
//
U32 Camera::Notify (U32 message, void *param)
{
	MSG *msg = (MSG *) param;

	switch (message)
	{
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		if(cameraMode != CAMERA_NOMODE)
		{
			cameraMode = CAMERA_NOMODE;
			ShowCursor(true);
			ReleaseCapture();
		}
		break;


	case WM_RBUTTONDOWN:
		if(GetKeyState(VK_LCONTROL)>> 8)
		{
			if(cameraMode == CAMERA_NOMODE)
			{
				SetCapture(previewWin);
				cameraMode = CAMERA_PAN;
				GetCursorPos(&cursorCenter);
				ShowCursor(false);
				lastTick = GetTickCount();
			}
		}
		break;
	case WM_LBUTTONDOWN:
		if(GetKeyState(VK_LCONTROL)>> 8)
		{
			if(cameraMode == CAMERA_NOMODE)
			{
				SetCapture(previewWin);
				cameraMode = CAMERA_ROTATE;
				GetCursorPos(&cursorCenter);
				ShowCursor(false);
				lastTick = GetTickCount();
			}
		}
		break;
	case WM_MBUTTONDOWN:
		if(GetKeyState(VK_LCONTROL)>> 8)
		{
			if(cameraMode == CAMERA_NOMODE)
			{
				SetCapture(previewWin);
				cameraMode = CAMERA_ZOOM;
				GetCursorPos(&cursorCenter);
				ShowCursor(false);
				lastTick = GetTickCount();
			}
		}
		break;
/*	case WM_MOUSEWHEEL:
		{
			short fwKeys = LOWORD(msg->wParam);         // key flags
			short zDelta = (short) HIWORD(msg->wParam); // wheel rotation
			short xPos = (short) LOWORD(msg->lParam);   // horizontal position of pointer
			short yPos = (short) HIWORD(msg->lParam);   // vertical position of pointer
 
			if( zDelta > 0 )
				ZoomCamera( 0.1 );
			else
				ZoomCamera( -0.1 );
		}
		break;
*/	}

	return GR_OK;
}
//----------------------------------------------------------------------//
//
void Camera::SetHardpointMode(IEffectTarget * target,U32 _hpIndex)
{
	if(!hpTarget)
	{
		target->InitMovieLights();
		lastK = transform.translation-orbitPoint;
	}
	bSmoothHardpointMode = false;
	hpTarget = NULL;
	if(target)
	{
		hpTarget = target;
		if(hpTarget)
		{
/*			IGrannyInstance * inst = hpTarget->GetMesh();
			if(inst)
			{
				hpIndex = _hpIndex;
			}
			else
*/			{
				hpTarget = NULL;
			}
		}
	}
}
//----------------------------------------------------------------------//
//
void Camera::SetHardpointModeSmooth(IEffectTarget * target,U32 hpIndex, SINGLE gameTimeEnd)
{
	if(!hpTarget)
	{
		target->InitMovieLights();
		lastK = transform.translation-orbitPoint;
	}
	SetHardpointMode(target,hpIndex);
	SINGLE gameTime = PreviewWin::GetGameTime();
	if(gameTime < gameTimeEnd)
	{
		smoothTransStart = transform;
		smoothEndTime = gameTimeEnd;
		smoothStartTime = gameTime;
		bSmoothHardpointMode = true;
	}
}
//----------------------------------------------------------------------//
//
void Camera::EndHardpointMode()
{
	if(hpTarget)
	{
		hpTarget->DeleteMovieLights();
		hpTarget = NULL;

		transform.translation = orbitPoint+lastK;
		Vector k = lastK;
		k.fast_normalize();
		Vector j;
		if(k.x != 0 && k.y != 0)
			j = Vector(0,0,1);
		else
			j = Vector(0,1,0);

		Vector i = cross_product(j,k);
		i.fast_normalize();
		j = cross_product(k,i);
		transform.set_i(i);
		transform.set_j(j);
		transform.set_k(k);
	}
}
//----------------------------------------------------------------------//
//
void Camera::EndHardpointModeSmooth(SINGLE gameTimeEnd)
{
	if(hpTarget)
	{
		hpTarget->DeleteMovieLights();
		hpTarget = NULL;
	}
	SINGLE gameTime = PreviewWin::GetGameTime();
	if(gameTime < gameTimeEnd)
	{
		smoothTransStart = transform;
		smoothEndTime = gameTimeEnd;
		smoothStartTime = gameTime;
		bSmoothHardpointMode = true;
	}	
}
//----------------------------------------------------------------------//
//
void Camera::ResetClipPlane()
{
	nearClipSetting = 5000;
	farClipSetting = 800000;

	set_near_plane_distance(nearClipSetting);
	set_far_plane_distance(farClipSetting);
}
//----------------------------------------------------------------------//
//
void Camera::SetClipPlaneNear(SINGLE set)
{
	nearClipSetting = set;
	set_near_plane_distance(nearClipSetting);
}
//----------------------------------------------------------------------//
//
void Camera::SetClipPlaneFar(SINGLE set)
{
	farClipSetting = set;
	set_far_plane_distance(farClipSetting);
}
//----------------------------------------------------------------------//
//
void Camera::CameraShake(SINGLE durration, SINGLE power)
{
	shakeTimeLeft = durration;
	shakeTimeTotal = durration;
	shakeNoise = power;
}
//----------------------------------------------------------------------//
//
void Camera::Update()
{
	SINGLE renderTime = PreviewWin::GetRenderTime();
	if(renderTime >= shakeTimeLeft)
	{
		shakeTimeLeft = 0;
	}
	else
	{
		shakeTimeLeft -= renderTime;
		SINGLE noiseSize = shakeNoise*(shakeTimeLeft/shakeTimeTotal);
		Vector shakeNoise(((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize,
				((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize,
				((((SINGLE)(rand()%1000))/1000.0f)*2*noiseSize)-noiseSize);

		Vector newCenter = trueOrbitPoint+shakeNoise;
		Vector diff = transform.translation-orbitPoint;
		orbitPoint =newCenter;
		transform.translation = newCenter+diff;
		SetOrientation();
	}

	if(cameraMode != CAMERA_NOMODE)
	{
		if(cameraMode == CAMERA_PLANE_PAN && (GetKeyState(VK_UP) >> 8 || GetKeyState(VK_DOWN) >> 8 || GetKeyState(VK_LEFT) >> 8 || GetKeyState(VK_RIGHT) >> 8))
		{
			U32 nextTick = GetTickCount();
			SINGLE numTicks = nextTick-lastTick;
			numTicks /= 1500;
			lastTick = nextTick;
			SINGLE zDist = (transform.translation-orbitPoint).fast_magnitude()*10;
			if(GetKeyState(VK_UP) >> 8)
			{
				PlaneScroll(-numTicks*data.panRate*zDist,0);
			}
			if(GetKeyState(VK_DOWN) >> 8)
			{
				PlaneScroll(numTicks*data.panRate*zDist,0);
			}
			if(GetKeyState(VK_LEFT) >> 8)
			{
				PlaneScroll(0,-numTicks*data.panRate*zDist);
			}
			if(GetKeyState(VK_RIGHT) >> 8)
			{
				PlaneScroll(0,numTicks*data.panRate*zDist);
			}
			InvalidateRect(previewWin,0,false);
		}
		else if(GetKeyState(VK_LCONTROL) >> 8)
		{
			POINT newCursor;
			GetCursorPos(&newCursor);
			U32 nextTick = GetTickCount();
			SINGLE numTicks = nextTick-lastTick;
			numTicks /= 1500;
			lastTick = nextTick;
			
			if(newCursor.x != cursorCenter.x || newCursor.y != cursorCenter.y )
			{
				S32 xdiff = cursorCenter.x - newCursor.x;
				S32 ydiff = cursorCenter.y - newCursor.y;
				switch (cameraMode)
				{
					case CAMERA_PAN:
						PanCamera(numTicks*xdiff*data.panRate,-numTicks*ydiff*data.panRate);
						break;
					case CAMERA_ROTATE:
						OrbitCamera(numTicks*xdiff*data.rotateRate,numTicks*ydiff*data.rotateRate);
						break;
					case CAMERA_ZOOM:
						ZoomCamera(numTicks*ydiff*data.zoomRate);
						break;
				}
				InvalidateRect(previewWin,0,false);
				SetCursorPos(cursorCenter.x,cursorCenter.y);
			}
		}
		else
		{
			if(cameraMode != CAMERA_PLANE_PAN)
			{
				ShowCursor(true);
				ReleaseCapture();
			}
			cameraMode = CAMERA_NOMODE;
		}
	}
	else if((GetKeyState(VK_UP) >> 8 || GetKeyState(VK_DOWN) >> 8 || GetKeyState(VK_LEFT) >> 8 || GetKeyState(VK_RIGHT) >> 8) && (GetForegroundWindow() == previewWin) )
	{
		lastTick = GetTickCount();
		cameraMode = CAMERA_PLANE_PAN;
	}

	if(hpTarget)//force the orientation to be set if we are tagging a target
		SetOrientation();

}
//----------------------------------------------------------------------//
//
void Camera::PlaneScroll(SINGLE vert,SINGLE horz)
{
	Vector hVect = transform.get_k();
//	Vector vVect = transform.get_j();
	Vector up(0,0,1);
	Vector planeSide = cross_product(up,hVect);
	planeSide.normalize();
	Vector planeForward = cross_product(planeSide,up);
	planeForward.normalize();
	
	transform.translation += (planeSide*horz)+(planeForward*vert);
	orbitPoint += (planeSide*horz)+(planeForward*vert);
	trueOrbitPoint += (planeSide*horz)+(planeForward*vert);
	SetOrientation();	
}
//----------------------------------------------------------------------//
//
void Camera::PanCamera(SINGLE horz,SINGLE vert)
{
	Vector hVect = transform.get_i();
	Vector vVect = transform.get_j();
	transform.translation += (hVect*horz)+(vVect*vert);
	orbitPoint += (hVect*horz)+(vVect*vert);
	trueOrbitPoint += (hVect*horz)+(vVect*vert);
	SetOrientation();
}
//----------------------------------------------------------------------//
//
void Camera::OrbitCamera(SINGLE yawChange, SINGLE pitchChange)
{
	Transform rot;
	rot.set_identity();
	rot.set_z_rotation(yawChange*MUL_DEG_TO_RAD);
	Vector offset = transform.translation - orbitPoint;

	offset = rot.rotate(offset);

	TRANSFORM pitchRot;
	pitchRot.set_identity();
	pitchRot.set_orientation(transform);
	SINGLE pitchOrg = pitchRot.get_pitch_zbased()*MUL_RAD_TO_DEG;
	SINGLE newPitch = pitchOrg+pitchChange;
	if(newPitch > -5.0f)
		pitchChange = (-5.0)-pitchOrg;
	else if(newPitch < -85.0f)
	{
		pitchChange = 85.0f+pitchOrg;
		if(pitchChange >0.0)
			pitchChange = 0.0;
	}
	pitchRot.rotate_about_i(pitchChange*MUL_DEG_TO_RAD);
	pitchRot = mul((Matrix)pitchRot,(Matrix)(transform.get_inverse()));

	offset = pitchRot.rotate(offset);
	
	transform.translation = orbitPoint+offset;
	SetOrientation();
}
//----------------------------------------------------------------------//
//
void Camera::ZoomCamera(SINGLE zoom)
{
	Vector vect = -transform.get_k();
	SINGLE dist = (transform.translation-orbitPoint).fast_magnitude();
	if(zoom > 0 && zoom > dist-data.minZoom)
		zoom = dist-data.minZoom;
	if(zoom < 0 && (-zoom) > data.maxZoom-dist)
		zoom = -(data.maxZoom-dist);
	transform.translation += (vect*zoom);
	SetOrientation();
}
//----------------------------------------------------------------------//
//
void Camera::ResetCamera(BOOL32 updates)
{
	resetCamera(updates);
}
//----------------------------------------------------------------------//
//
void Camera::getFrustum()
{
/*	D3DXMATRIXA16 proj = RENDERER->GetProjectionMatrix();;
	D3DXMATRIXA16 world = RENDERER->GetWorldMatrix();
	D3DXMATRIXA16 view = RENDERER->GetViewMatrix();;
	D3DXMATRIXA16 modl;
	D3DXMatrixMultiply(&modl,&world,&view);

	D3DXMATRIXA16 clip;
	D3DXMatrixMultiply(&clip,&modl,&proj);

	SINGLE t;

	// Extract the numbers for the RIGHT plane 
	frustum[0][0] = clip.m[0][ 3] - clip.m[0][ 0];
	frustum[0][1] = clip.m[1][ 3] - clip.m[1][ 0];
	frustum[0][2] = clip.m[2][3] - clip.m[2][ 0];
	frustum[0][3] = clip.m[3][3] - clip.m[3][0];

	// Normalize the result 
	t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
	frustum[0][0] /= t;
	frustum[0][1] /= t;
	frustum[0][2] /= t;
	frustum[0][3] /= t;

	// Extract the numbers for the LEFT plane 
	frustum[1][0] = clip.m[0][ 3] + clip.m[0][ 0];
	frustum[1][1] = clip.m[1][ 3] + clip.m[1][ 0];
	frustum[1][2] = clip.m[2][ 3] + clip.m[2][ 0];
	frustum[1][3] = clip.m[3][ 3] + clip.m[3][ 0];

	// Normalize the result 
	t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
	frustum[1][0] /= t;
	frustum[1][1] /= t;
	frustum[1][2] /= t;
	frustum[1][3] /= t;

	// Extract the BOTTOM plane 
	frustum[2][0] = clip.m[0][ 3] + clip.m[0][ 1];
	frustum[2][1] = clip.m[1][ 3] + clip.m[1][ 1];
	frustum[2][2] = clip.m[2][ 3] + clip.m[2][ 1];
	frustum[2][3] = clip.m[3][ 3] + clip.m[3][ 1];

	// Normalize the result 
	t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
	frustum[2][0] /= t;
	frustum[2][1] /= t;
	frustum[2][2] /= t;
	frustum[2][3] /= t;

	// Extract the TOP plane 
	frustum[3][0] = clip.m[0][ 3] - clip.m[0][ 1];
	frustum[3][1] = clip.m[1][ 3] - clip.m[1][ 1];
	frustum[3][2] = clip.m[2][ 3] - clip.m[2][ 1];
	frustum[3][3] = clip.m[3][ 3] - clip.m[3][ 1];

	// Normalize the result 
	t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
	frustum[3][0] /= t;
	frustum[3][1] /= t;
	frustum[3][2] /= t;
	frustum[3][3] /= t;

	// Extract the FAR plane 
	frustum[4][0] = clip.m[0][ 3] - clip.m[0][ 2];
	frustum[4][1] = clip.m[1][ 3] - clip.m[1][ 2];
	frustum[4][2] = clip.m[2][ 3] - clip.m[2][ 2];
	frustum[4][3] = clip.m[3][ 3] - clip.m[3][ 2];

	// Normalize the result 
	t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
	frustum[4][0] /= t;
	frustum[4][1] /= t;
	frustum[4][2] /= t;
	frustum[4][3] /= t;

	// Extract the NEAR plane 
	frustum[5][0] = clip.m[0][ 3] + clip.m[0][ 2];
	frustum[5][1] = clip.m[1][ 3] + clip.m[1][ 2];
	frustum[5][2] = clip.m[2][ 3] + clip.m[2][ 2];
	frustum[5][3] = clip.m[3][ 3] + clip.m[3][ 2];

	// Normalize the result 
	t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
	frustum[5][0] /= t;
	frustum[5][1] /= t;
	frustum[5][2] /= t;
	frustum[5][3] /= t;*/
}
//----------------------------------------------------------------------//
//
void Camera::RecenterCamera(Vector newCenter)
{
	trueOrbitPoint = newCenter;
	Vector diff = transform.translation-orbitPoint;
	orbitPoint =newCenter;
	transform.translation = newCenter+diff;
	SetOrientation();
}
//--------------------------------------------------------------------------//
//
void Camera::resetCamera (BOOL32 bUpdate)
{
	nearClipSetting = 5000;
	farClipSetting = 800000;

	data.FOV_x = 30.0;
	data.FOV_y = 0.0;
	data.orbitPoint = Vector(0,0,0);
	trueOrbitPoint = Vector(0,0,0);
	data.position.x = 1; data.position.y = 1; data.position.z = 2;
	data.position.normalize();
	data.position = data.orbitPoint+(data.position*data.maxZoom);

	SetStateInfo(&data, bUpdate);
}

//--------------------------------------------------------------------------//
//
void Camera::SetCameraDefaults (struct CAMERA_DATA & cameraData) const
{
	//sector uses this to stabilize the camera to the most recent defaults

	cameraData.FOV_x = 30.0;
	cameraData.FOV_y = 0.0;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::CreateViewer (void)
{
	resetCamera(0);

	data.rotateRate = 45;    // degrees per second
	data.verticalRotateRate = 45;
	data.zoomRate = 10000.0f;
	data.panRate = 1000.0f;

	data.maxZoom = 100000;
	data.minZoom = 7000;

	return false;
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
	x = 0;
	y = 0;

	return 1;
}
//--------------------------------------------------------------------------//
//
Vector Camera::ScreenToPoint(SINGLE & x, SINGLE & y)
{
	Vector rayDir;

	screen_to_point(rayDir, x, y);

	return rayDir;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::UpdateViewer (void)
{
	data.FOV_x = fovx*2.0;
	data.FOV_y = fovy*2.0;
	data.position = transform.translation;
	data.orbitPoint = orbitPoint;

	set_near_plane_distance(nearClipSetting);
	set_far_plane_distance(farClipSetting);
	
	SetStateInfo(&data, 0);
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

	transform.translation = data.position;
	orbitPoint = data.orbitPoint;
	trueOrbitPoint = orbitPoint;

	SetOrientation(update);

	return 1;
}

static BOOL32 bPerspective=0;
static BOOL32 bNullView=0;
static BOOL32 bIdentityView=0;
static bool bNullPane = false;
//--------------------------------------------------------------------------//
//
GENRESULT Camera::SetPerspective ()
{
	bNullPane = false;
	ppane->x0 = 0;
	ppane->x1 = SCREENRESX - 1;
	ppane->y0 = 0;
	ppane->y1 = SCREENRESY - 1;
	set_pane(ppane);
	SetHorizontalFOV(data.FOV_x, 0);
	
	PIPE->set_viewport(ppane->x0,ppane->y0,ppane->x1-ppane->x0+1,ppane->y1-ppane->y0+1);
	PIPE->set_perspective(fovy, aspect, znear, zfar);
	getFrustum();	
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
	PIPE->set_modelview(to_view);

	bIdentityView = FALSE;
	getFrustum();

	return GR_OK;
}

//--------------------------------------------------------------------------
//
void OrthoView (const ViewRect *pane)
{
	// TO DO : make orthoview work with the DX9 renderer
	Transform trans;
	trans.translation.x = -0.5f;
	trans.translation.y = -0.5f;
	// BATCH->set_modelview(trans);

	if (pane == 0 && bNullPane == false)
	{
		bNullPane = true;
		// BATCH->set_ortho(0,SCREENRESX,SCREENRESY,0,-1,+1);//0,MAX_ORTHO_DEPTH);
		// BATCH->set_viewport(0,0,SCREENRESX,SCREENRESY);
	}
	else if (pane != 0)
	{
		bNullPane = false;
//		RENDERER->SetOrtho(pane->x0,pane->x1+1,pane->y1+1,pane->y0,-1,1);//0,MAX_ORTHO_DEPTH);		// left, right, bottom, top, near, far
		// RENDERER->SetViewport(pane->x0,pane->y0,pane->x1-pane->x0+1,pane->y1-pane->y0+1);
	}

	// BATCH->set_render_state( D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SetLookAtPosition (const Vector &newPos)
{
	trueOrbitPoint = newPos;
	orbitPoint = newPos;
	SetOrientation();

	return 1;
}
//--------------------------------------------------------------------------//
//
Vector Camera::GetLookAtPosition (void) const
{
	return orbitPoint;
}
//--------------------------------------------------------------------------//
//
BOOL32 Camera::SnapToTargetRotation (void)
{
	return TRUE;
}
//--------------------------------------------------------------------------//
// frameTime is in milliseconds
//
void Camera::updateZoom (U32 frameTime)
{

}
//--------------------------------------------------------------------------//
// -zDelta = rolled toward user, +zDelta = rolled away from user
//
void Camera::onMouseWheel (S32 zDelta)
{
}
//--------------------------------------------------------------------------//
//
SINGLE Camera::GetCameraLOD (void)
{
	return 1.0;
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
		
		float center_distance = vcenter.fast_magnitude();
		
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
	for(U32 i = 0; i < 6; ++i )
      if( frustum[i][0] * pos.x + frustum[i][1] * pos.y + frustum[i][2] * pos.z + frustum[i][3] <= -radius_3d )
         return false;
   return true;

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
	camera->SetOrientation(0);

	return camera;
}

struct _camera : GlobalComponent
{
	Camera * camera;

	virtual void Startup (void)
	{
		CAMERA_INIT info;
		Vector pos,look;
		ViewRect pane;

		memset(&info, 0, sizeof(info));
		pos.x = pos.y = 0;
		pos.z = 9000;
		look = Vector(0,0,0);
		pane.x0 = 0;
		pane.x1 = SCREENRESX;
		pane.y0 = 0;
		pane.y1 = SCREENRESY;
		info.pane = &pane;
//		pane.x0 = minX;

		info.flags = CIF_PANE|CIF_HFOV | CIF_POS | CIF_LOOK;

		info.pos = &pos;
		info.lookPos = &look;
		info.hfov = 8;
		CAMERA = camera = CreateCamera(&info);
		MAINCAM = camera;
		MAINCAM->AddRef();
		AddToGlobalCleanupList((IDAComponent **) &MAINCAM);
		AddToGlobalCleanupList((IDAComponent **) &CAMERA);
	}

	virtual void Initialize (void)
	{
		camera->CreateViewer();
	}
};
static _camera camera;

//--------------------------------------------------------------------------//
//-----------------------------End Camera.cpp-------------------------------//
//--------------------------------------------------------------------------//
