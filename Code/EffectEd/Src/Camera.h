#ifndef CAMERA_H
#define CAMERA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                Camera.h                                  //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Tmauer $

   $Header: /EffectEd/Src/Camera.h 2     10/23/03 10:39p Tmauer $

	Camera resource


	//------------------------------
	//
	struct ViewRect * IBaseCamera::GetPane (void) const
		RETURNS:
			Address of pane structure being used by Camera

	//------------------------------
	//
	BOOL32 IBaseCamera::SetPane (struct ViewRect *pane, BOOL32 update=1)
		INPUT:
			pane: Address of a pane structure.
			update: Set TRUE if change is to take place immediately
		RETURNS:
			TRUE
		OUTPUT:
			Makes a copy of the PANE structure pointed to by 'pane'.

	//------------------------------
	//
	BOOL32 IBaseCamera::SetPaneRef (struct ViewRect *pane, BOOL32 update=1)
		INPUT:
			pane: Address of a pane structure.
			update: Set TRUE if change is to take place immediately
		RETURNS:
			TRUE
		OUTPUT:
			Stores the address of the PANE structure pointed to by 'pane'.

	//------------------------------
	//
	const class Transform * IBaseCamera::GetTransform (void) const
		RETURNS:
			Address of the transform class that converts Camera coords to world coordinate coords

	//------------------------------
	//
	const class Transform * IBaseCamera::GetInverseWorldTransform (void) const   
		RETURNS:
			Address of the transform class that converts world coords to rotated_world coords

	//------------------------------
	//
	const class Transform * IBaseCamera::GetWorldTransform (void) const  
		RETURNS:
			Address of the transform class that converts rotated_world coords to world coords

	//------------------------------
	//
	const class Transform * IBaseCamera::GetInverseSectorTransform (void) const  
		RETURNS:
			Address of the transform class that converts world coords to rotated_sector coords

	//------------------------------
	//
	const class Transform * IBaseCamera::GetSectorTransform) (void) const  
		RETURNS:
			Address of the transform class that converts rotated_sector coords to world coords

	//------------------------------
	//
	SINGLE IBaseCamera::GetHorizontalFOV (void) const
		RETURNS:
			Returns the effective horizontal field of view (in degrees) 

	//------------------------------
	//
	BOOL32 IBaseCamera::SetHorizontalFOV (SINGLE fx, BOOL32 update=1)
		INPUT:
			fx: Effective horizontal field of view (in degrees)
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE

	//------------------------------
	//
	SINGLE IBaseCamera::GetVerticalFOV (void) const
		RETURNS:
			Returns the effective vertical field of view (in degrees) 

	//------------------------------
	//
	BOOL32 IBaseCamera::SetVerticalFOV (SINGLE fy, BOOL32 update=1)
		INPUT:
			fx: Effective vertical field of view (in degrees)
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE

	//------------------------------
	//
	SINGLE IBaseCamera::GetWorldRotation (void) const
		RETURNS:
			Current world rotation (in degrees).

	//------------------------------
	//
	S32 IBaseCamera::GetCameraID (void) const
		RETURNS:
			Camera index (assigned by Engine.)

	//------------------------------
	//
	void IBaseCamera::GetOrientation (SINGLE * pitch, SINGLE * roll, SINGLE * yaw) const
		OUTPUT:
			pitch: rotation about i (in degrees)
			roll:  rotation about -k (in degrees)
			yaw:   rotation about j  (in degrees)
		NOTES:
			Returns the current camera rotations.

	//------------------------------
	//
	BOOL32 IBaseCamera::SetOrientation (SINGLE pitch, SINGLE roll, SINGLE yaw, BOOL32 update=1)
		INPUT:
			pitch: rotation about i (in degrees)
			roll:  rotation about -k (in degrees)
			yaw:   rotation about j  (in degrees)
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE
		OUTPUT:
			Sets the new orientation for the camera. The transform matrices are recalculated.

	//------------------------------
	//
	class Vector IBaseCamera::GetPosition (void) const
		RETURNS:
			World coordinates for the camera.

	//------------------------------
	//
	BOOL32 IBaseCamera::SetRotatedPosition (const class Vector * newPos, BOOL32 update=1)
		INPUT:
			newPos: Address of vector to use as new rotated_world position of the camera.
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE
		OUTPUT:
			Sets the position of the camera, in rotated_world coordinates.

	//------------------------------
	//
	class Vector IBaseCamera::GetRotatedPosition (void) const
		RETURNS:
			Rotated world coordinates for the camera.

	//------------------------------
	//
	BOOL32 IBaseCamera::SetPosition (const class Vector * newPos, BOOL32 update=1)
		INPUT:
			newPos: Address of vector to use as new world coordinate position of the camera.
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE
		OUTPUT:
			Sets the position of the camera, in world coordinates.

	//------------------------------
	//
	BOOL32 IBaseCamera::MoveForward (SINGLE distance, BOOL32 update=1)
		INPUT:
			distance: Scalar amount to move forward. (in world units) (Negative amount causes
				camera to move backward).
			update: TRUE if the change should take place immediately.
		RETURNS:
			TRUE
		OUTPUT:
			Moves the camera along the look vector (-k).

	//------------------------------
	//
	TRANSRESULT IBaseCamera::PointToScreen (const Vector &point, S32 *pane_X, S32 *pane_Y, const Transform *object_to_world) const
		INPUT:
			point: Point in 3D space to transform into screen space.
			object_to_world: If non-null, transform in concatenated onto Camera's transform,
				in order to perform transform from object space to screen space.
		RETURNS:
			Result of transform. If result is BEHIND_CAMERA, 2D x-y coordinates are not returned.
			*pane_X: Horizontal coordinate, relative to the Camera's pane.
			*pane_Y: Vertical coordinate, relative to the Camera's pane.

	//------------------------------
	//  returns ROTATED WORLD COORDINATES, meaning map coordinates
	//  meaning p0.x is the smallest x
	//
	BOOL32 IBaseCamera::PaneToPoints (Vector & p0, Vector & p1, Vector & p2, Vector & p3) const
		RETURNS:
			TRUE
		OUTPUT:
			p0: World coordinate of top-left corner of the view area
			p1: World coordinate of top-right corner of the view area
			p2: World coordinate of bottom-right corner of the view area
			p3: World coordinate of bottom-left corner of the view area


	//------------------------------
	//
	BOOL32 IBaseCamera::ScreenToPoint (SINGLE & x, SINGLE & y, SINGLE z=0.0) const
		INPUT:
			x, y: Screen coordinates.
			z: z coordinate in world 3D space.
		RETURNS:
			TRUE
		OUTPUT:
			Converts the screen coordinates and a world z value into world coordinates.
		
	//------------------------------
	//
	//------------------------------
	//
	BOOL32 IBaseCamera::SetLookAtPosition (const Vector &position);
		INPUT:
			position: World coordinates that camera should look at.
		RETURNS:
			TRUE:
		OUTPUT:
			Positions the camera to look at the specified position.

	//------------------------------
	//
	BOOL32 IBaseCamera::SnapToTargetRotation (void);
		OUTPUT:
			Sets camera to targetRotation.

*/
//--------------------------------------------------------------------------//

//--------------------------------------------------------------------------//
// flags used in CAMERA_INIT	// describe the meaning of CAMERA_INIT members

#define CIF_PANE		0x00000001		// make a copy of pane passed in
#define CIF_PANEREF		0x00000002		// keep a pointer to the pane (don't make a copy)
#define CIF_POS         0x00000004		// pos vector is valid (set the initial position of the camera)
#define CIF_HFOV		0x00000008		// set the horizontal field of view
#define CIF_VFOV        0x00000010		// set the vertical field of view
#define CIF_LOOK		0x00000020		// set the point to look at
#define CIF_MENUID		0x00000100		// menu id is valid, create a menu entry
//

//--------------------------------------------------------------------------//
//
struct CAMERA_INIT
{
	U32 flags;
	
	struct ViewRect *		pane;
	class Vector *		pos;
	class Vector *		lookPos;
	SINGLE				hfov;
	SINGLE				vfov;
	U32					viewerMenuID;
};


enum TRANSRESULT
{
	BEHIND_CAMERA,
	IN_PANE,
	OUT_PANE
};

enum CameraMode
{
	CAMERA_NOMODE,
	CAMERA_PAN,
	CAMERA_ROTATE,
	CAMERA_ZOOM,
	CAMERA_ZOOMWHEEL,
	CAMERA_PLANE_PAN,
};

class Vector;
//--------------------------------------------------------------------------//
//--------------------------------------------------------------------------//

struct DACOM_NO_VTABLE IBaseCamera : public IDAComponent
{
	DEFMETHOD_(struct ViewRect *,GetPane) (void) const = 0;

	DEFMETHOD_(BOOL32,SetPane) (struct ViewRect *pane, BOOL32 update=1) = 0;

	DEFMETHOD_(BOOL32,SetPaneRef) (struct ViewRect *pane, BOOL32 update=1) = 0;

	DEFMETHOD_(const class Transform *,GetTransform) (void) const = 0;

	DEFMETHOD_(const class Transform *,GetInverseTransform) (void) const = 0;  /* world to camera coordinates */

	DEFMETHOD_(SINGLE,GetHorizontalFOV) (void) const = 0;

	DEFMETHOD_(BOOL32,SetHorizontalFOV) (SINGLE fx, BOOL32 update=1) = 0;

	DEFMETHOD_(SINGLE,GetVerticalFOV) (void) const = 0;

	DEFMETHOD_(BOOL32,SetVerticalFOV) (SINGLE fy, BOOL32 update=1) = 0;

	DEFMETHOD_(SINGLE,GetWorldRotation) (void) const = 0;

	DEFMETHOD_(void,GetOrientation) (SINGLE * pitch, SINGLE * roll, SINGLE * yaw) const = 0;

	DEFMETHOD_(BOOL32,SetOrientation) (BOOL32 update=1) = 0;

	DEFMETHOD_(class Vector,GetPosition) (void) const = 0;

	DEFMETHOD_(BOOL32,SetRotatedPosition) (const class Vector * newPos, BOOL32 update=1) = 0;

	DEFMETHOD_(class Vector,GetRotatedPosition) (void) const = 0;

	DEFMETHOD_(BOOL32,SetPosition) (const class Vector * newPos, BOOL32 update=1) = 0;

	DEFMETHOD_(BOOL32,MoveForward) (SINGLE distance, BOOL32 update=1) = 0;

	DEFMETHOD_(TRANSRESULT,PointToScreen) (const Vector &point, S32 *pane_X, S32 *pane_Y, const Transform *object_to_world=0) const = 0;

	DEFMETHOD_(BOOL32,PaneToPoints) (Vector & top, Vector & bottom, Vector & left, Vector & right) const = 0;

	DEFMETHOD_(BOOL32,ScreenToPoint) (SINGLE & x, SINGLE & y, SINGLE z=0.0) const = 0;

	DEFMETHOD_(Vector, ScreenToPoint) (SINGLE & x, SINGLE & y) = 0;

	DEFMETHOD(SetPerspective) () = 0;
	
	virtual GENRESULT SetModelView(const class Transform *object_to_world = 0) = 0;

	DEFMETHOD_(BOOL32,SetLookAtPosition) (const Vector &position) = 0;

	virtual Vector GetLookAtPosition (void) const = 0;

	DEFMETHOD_(BOOL32,SnapToTargetRotation) (void) = 0;

	DEFMETHOD_(SINGLE,GetCameraLOD) (void) = 0;

	virtual bool SphereInFrustrum(const Vector &pos,float radius_3d,float & cx,float & cy,float & radius_2d,float & depth);

	virtual bool SphereInFrustrumFast(const Vector &pos,float radius_3d);

	// sets the approved defaults in cameraData, (overrides the registry)
	DEFMETHOD_(void,SetCameraDefaults) (struct CAMERA_DATA & cameraData) const = 0;

	// resets all camera numbers back to their default values
	DEFMETHOD_(void,SetCameraToDefaults) (void) = 0;

	virtual void Update() = 0;

	virtual void PanCamera(SINGLE horz,SINGLE vert) = 0;

	virtual void OrbitCamera(SINGLE yawChange, SINGLE pitchChange) = 0;

	virtual void ZoomCamera(SINGLE zoom) = 0;

	virtual enum CameraMode GetCameraMode() = 0;

	virtual void ResetCamera(BOOL32 updates) = 0;

	virtual void RecenterCamera(Vector newCenter) = 0;

	virtual U32 Notify(U32 message, void *param = 0) = 0;

	virtual void SetHardpointMode(struct IEffectTarget * target,U32 hpIndex) = 0;

	virtual void SetHardpointModeSmooth(struct IEffectTarget * target,U32 hpIndex, SINGLE gameTimeEnd) = 0;

	virtual void EndHardpointMode() = 0;

	virtual void EndHardpointModeSmooth(SINGLE gameTimeEnd) = 0;

	virtual void ResetClipPlane() = 0;
	
	virtual void SetClipPlaneNear(SINGLE set) = 0;

	virtual void SetClipPlaneFar(SINGLE set) = 0;

	virtual void CameraShake(SINGLE durration, SINGLE power) = 0;
};

//--------------------------------------------------------------------------//
//------------------------------End Camera.h--------------------------------//
//--------------------------------------------------------------------------//
#endif