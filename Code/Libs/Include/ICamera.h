#ifndef ICAMERA_H
#define ICAMERA_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                ICamera.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/ICamera.h 5     4/28/00 11:57p Rmarr $
*/			    
//---------------------------------------------------------------------------

/*
	//-------------------------------------------
	//
	Vector ICamera::get_position (void) const;
		RETURNS:
			position: 3D position of the light, in WORLD coordinates.

	//-------------------------------------------
	//
	Transform ICamera::get_transform (class Transform & transform) const;
		RETURNS:
			transform:	Orientation of the light.

    //-------------------------------------------
	//
	const ViewRect * ICamera::get_pane (void) const;
		RETURNS:
			ViewRect representing screen area used by camera.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_fovx (void) const;
		RETURNS:
			1/2 of the horizontal field of view.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_fovy (void) const;
		RETURNS:
			1/2 of the vertical field of view.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_znear (void) const;
		RETURNS:
			Near plane distance.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_zfar (void) const;
		RETURNS:
			Far plane distance.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_aspect (void) const;
		RETURNS:
			near_plane_w / near_plane_h. THIS HAS NOTHING TO DO WITH PIXELS.
			IF WE NEED A PIXEL ASPECT RATIO, IT'S A SEPARATE THING.

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_hpc (void) const;
		RETURNS:
			Scalar from world units to pixels in x direction at znear

    //-------------------------------------------
	//
	virtual SINGLE ICamera::get_vpc (void) const;
		RETURNS:
			Scalar from world units to pixels in y direction at znear
  	
	//-------------------------------------------
	//
	Transform ICamera::get_inverse_transform () const;
		RETURNS:
			transform:	get_transform().get_inverse()
			The implementation can handle this smartly so that the inverse is calculated only when
		needed and stored for later.

	//-------------------------------------------
	//
	void ICamera::screen_to_point (Vector & world_vector, float screen_x, float screen_y) const;
		RETURNS:
			a vector in world space at znear

*/

#ifndef DACOM_H
#include "dacom.h"   
#endif

// Interface versioning macros
// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define ICAMERA_VERSION 1
#define IID_ICamera MAKE_IID("ICamera", 1)

// Declarations of classes whose internals we don't care about here in the header.
class Transform;
class Vector;

//------------------------------------------------------------------------------

typedef enum vis_state
{
	VS_UNKNOWN,
	VS_NOT_VISIBLE,
	VS_SUB_PIXEL,			// smaller than one pixel
	VS_PARTIALLY_VISIBLE,
	VS_FULLY_VISIBLE,
	VS_END = 0x7FFFFFFF
} vis_state; // the order here is important (inceasing from invisible to fully visible) -ms

//------------------------------------------------------------------------------

enum PSRESULT
{
   PS_VALID,
   PS_OFF_PANE,
   PS_Z_CLIPPED
};

//------------------------------------------------------------------------------

struct DACOM_NO_VTABLE ICamera : public IDAComponent
{
	virtual const Vector & COMAPI get_position (void) const = 0;

	virtual const Transform & COMAPI get_transform (void) const = 0;

	virtual const Vector & COMAPI get_look_pos(void) const = 0;
	
	virtual const struct ViewRect * COMAPI get_pane (void) const = 0;

	virtual SINGLE COMAPI get_fovx (void) const = 0;

	virtual SINGLE COMAPI get_fovy (void) const = 0;

	virtual SINGLE COMAPI get_znear (void) const = 0;

	virtual SINGLE COMAPI get_zfar (void) const = 0;

	virtual SINGLE COMAPI get_aspect (void) const = 0;

	virtual SINGLE COMAPI get_hpc (void) const = 0;

	virtual SINGLE COMAPI get_vpc (void) const = 0;

	virtual bool COMAPI point_to_screen (float & screen_x, float & screen_y, float & depth, const Vector & world_vector) const = 0;

	virtual void COMAPI screen_to_point (Vector & world_vector, float screen_x, float screen_y) const = 0;

	virtual vis_state COMAPI object_visibility (const Vector &view_pos, float radius) const = 0;

	virtual Transform COMAPI get_inverse_transform (void) const = 0;
};

//------------------------------------------------------------------------------
//-----------------------------END ICamera.h------------------------------------
//------------------------------------------------------------------------------

#endif