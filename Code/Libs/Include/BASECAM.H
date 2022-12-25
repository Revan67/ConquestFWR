#ifndef BASECAM_H
#define BASECAM_H
//--------------------------------------------------------------------------//
//                                                                          //
//                                Basecam.h                                 //
//                                                                          //
//               COPYRIGHT (C) 1997 BY DIGITAL ANVIL, INC.                  //
//                                                                          //
//--------------------------------------------------------------------------//
/*
   $Author: Rmarr $

   $Header: /Conquest/Libs/Include/BASECAM.H 5     4/28/00 11:57p Rmarr $
*/
//--------------------------------------------------------------------------//

#ifndef TCOMPONENT_H
#include "TComponent.h"
#endif

#ifndef VIEW2D_H
#include "View2d.h"  // for ViewRect
#endif

#ifndef ICAMERA_H
#include "ICamera.h"   
#endif

#ifndef DAVARIANT_H
#include "DAVariant.h"
#endif

#ifndef _INC_MATH
#include <math.h>
#endif

#ifndef _3DMATH_H
#include "3dmath.h"
#endif

#ifndef TSMARTPOINTER_H
#include "TSmartPointer.h"
#endif

#ifndef ENGINE_H
#include "Engine.h"
#endif

#include "fdump.h"

#ifndef FDUMP_H
#include "FDUMP.h"
#endif

#ifndef PI
  #define PI 3.1415926536
#endif

#ifndef MUL_DEG_TO_RAD
  #define MUL_DEG_TO_RAD ((2.0 * PI) / 360.0)
#endif

#ifndef MUL_RAD_TO_DEG
#define MUL_RAD_TO_DEG	(180.0 / PI)
#endif

typedef S32 INSTANCE_INDEX;		// also defined in Engine.h, ObjNode.h

#ifndef _INC_MALLOC
#include <malloc.h>		// for calloc
#endif

//
// HOW TO USE BASECAMERA PROPERTIES:
// 
// BaseCamera() defaults to a horizontal field of view of 90 degrees and a 
// vertical field of view corresponding to a full-screen 640x480 window with 
// square pixels, which happens to be about 74 degrees.
//
// You can independently change the fields of view by using 
//
// set_Horizontal_FOV(float(fov));
//
// and 
// 
// set_Vertical FOV(float(fov));
//
// CHANGING HORIZONTAL IN THIS WAY DOES NOT AFFECT VERTICAL AND VICE VERSA.
//
// IF you want to set the horizontal FOV and have the vertical FOV computed 
// based on the aspect ratio of your window, use the following:
//
// set_Horizontal_FOV(fov));
// float aspect = desired_width / desired_height;
// set_Horizontal_to_vertical_aspect(aspect);
// 
// Where "aspect" is the width of your view plane divided by its height. 
// Unless you're trying to achieve some weird warping effect, you can just 
// use the width of your pane divided by its height.
//
// Likewise, if you want to set the vertical and have the horizontal computed
// for you, use
//
// set_Vertical_FOV(fov);
// float aspect = desired_height / desired_width;
// set_Vertical_to_horizontal_aspect(aspect);
//

//
// Common camera representation
//

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
struct __baseCameraImpl : public ICamera
{
	//
	// public interfaces supported
	//
	
	BEGIN_DACOM_MAP_INBOUND(__baseCameraImpl)
	DACOM_INTERFACE_ENTRY(ICamera)
	DACOM_INTERFACE_ENTRY2(IID_ICamera,ICamera)
	END_DACOM_MAP()

	//
	// Physical viewport and "lens" attributes
	//

	ViewRect pane;

	float fovx;					// 1/2 of the horizontal field of view.
	float fovy;					// 1/2 of the vertical field of view.

	float znear;				// near plane distance.
	float zfar;					// far plane distance.

	float half_plane_w;			// width of (near=1.0) plane IN WORLD UNITS.
	float half_plane_h; 		// height of (near=1.0) plane IN WORLD UNITS.

	float aspect;				// near_plane_w / near_plane_h. THIS HAS NOTHING TO DO WITH PIXELS.
								// IF WE NEED A PIXEL ASPECT RATIO, IT'S A SEPARATE THING.

	float x_screen_center;		// Horizontal center of screen IN PIXELS.
	float y_screen_center;		// Vertical center of screen IN PIXELS.

	float hpc;					// scalar from world units to pixels in x direction. (pixels / meter)
	float vpc;					// scalar from world units to pixels in y direction.

// View plane normals (pointing away from view volume)
	Vector h_norm;				// normal for VIEW RIGHT plane
	Vector v_norm;				// normal for VIEW TOP plane

	__baseCameraImpl (void)
	{                   
	}

	__baseCameraImpl (ViewRect * pane)
	{                   
		__baseCameraInit(pane);
	}

	void __baseCameraInit (ViewRect * pane)
	{
		//
		// Initialize variables to reasonable values.
		//
		vpc =  1.0F;
		hpc = -1.0F;

		znear = 1.0f;
		zfar = 1e5;

		aspect = 4.0f / 3.0f;

		half_plane_w = 1.0f;
		half_plane_h = half_plane_w / aspect;

		set_pane(pane);

		compute_horizontal_quantities();
		compute_vertical_quantities();
	}

	/* ICamera methods */

	virtual const ViewRect * COMAPI get_pane (void) const
	{
		return &pane;
	}

	virtual SINGLE COMAPI get_fovx (void) const
	{
		return fovx;
	}

	virtual SINGLE COMAPI get_fovy (void) const
	{
		return fovy;
	}

	virtual SINGLE COMAPI get_znear (void) const
	{
		return znear;
	}

	virtual SINGLE COMAPI get_zfar (void) const
	{
		return zfar;
	}

	virtual SINGLE COMAPI get_aspect (void) const
	{
		return aspect;
	}

	virtual SINGLE COMAPI get_hpc (void) const
	{
		return hpc / znear;
	}
				  
	virtual SINGLE COMAPI get_vpc (void) const
	{
		return vpc / znear;
	}

	/* __baseCameraImpl methods */

	virtual ~__baseCameraImpl (void)
	{
	}

//
// Set new output pane for camera
//
	GENRESULT COMAPI set_pane(const ViewRect *_pane)
	{
		if (_pane)
		{
			pane = *_pane;
		}
		else
		{
			pane.x0 = 0;
			pane.y0 = 0;
			pane.x1 = 639;
			pane.y1 = 479;
		}
	//
	// Compute pane center, width.
	//
		x_screen_center = ((SINGLE) (pane.x1 - pane.x0)) * 0.5F;
		y_screen_center = ((SINGLE) (pane.y1 - pane.y0)) * 0.5F;

		hpc =  x_screen_center / half_plane_w;
		vpc = -y_screen_center / half_plane_h;

		return GR_OK;
	}

	GENRESULT COMAPI set_Horizontal_FOV (float value)
	{
		// Set horizontal FOV, recompute horizontal stuff.
		fovx = value * 0.5f;
		half_plane_w = tan(fovx * MUL_DEG_TO_RAD);

		compute_horizontal_quantities();

		// Recompute aspect based on new information.
		aspect = half_plane_w / half_plane_h;
		return GR_OK;
	}

	GENRESULT COMAPI set_Horizontal_to_vertical_aspect (float value)
	{
		// Set aspect, recompute vertical stuff..
		aspect = value;
		half_plane_h = half_plane_w / aspect;

		compute_vertical_quantities();
		return GR_OK;
	}

	GENRESULT COMAPI set_Vertical_FOV (float value)
	{
		// Set vertical FOV, recompute vertical.
		fovy = value * 0.5f;
		half_plane_h = tan(fovy * MUL_DEG_TO_RAD);

		compute_vertical_quantities();

		// Recompute aspect based on new information.
		aspect = half_plane_w / half_plane_h;
		return GR_OK;
	}

	GENRESULT COMAPI set_Vertical_to_horizontal_aspect (float value)
	{
		// Set aspect, recompute horizontal.
		aspect = 1.0f / value;
		half_plane_w = half_plane_h * aspect;

		compute_horizontal_quantities();
		return GR_OK;
	}

	GENRESULT COMAPI set_near_plane_distance (float value)
	{
		znear = value;
		set_Horizontal_FOV(fovx * 2.0f);
		set_Vertical_FOV(fovy * 2.0f);
		return GR_OK;
	}

	GENRESULT COMAPI set_far_plane_distance (float value)
	{
		zfar = value;
		return GR_OK;
	}
	
	GENRESULT COMAPI get_ICamera (void **ptr)
	{
		if(ptr)
		{
			*ptr = static_cast<ICamera *>(this);
			static_cast<ICamera *>(this)->AddRef();
			return GR_OK;
		}
		return GR_INVALID_PARMS;
	}

//
// ASSUMPTIONS: pane is correct. half_plane_w is correct. x_screen_center is correct.
//
	void compute_horizontal_quantities(void)
	{
		fovx = atan(half_plane_w) * MUL_RAD_TO_DEG;

		h_norm.set(1.0f, 0.0f, half_plane_w);	// points away from view volume
		h_norm.normalize();

		hpc = x_screen_center / half_plane_w;
	}

//
// ASSUMPTIONS: pane is correct. half_plane_h is correct. y_screen_center is correct.
//
	void compute_vertical_quantities(void)
	{
		fovy = atan(half_plane_h) * MUL_RAD_TO_DEG;

		v_norm.set(0.0f, 1.0f, half_plane_h);	// points away from view volume
		v_norm.normalize();

		vpc = -y_screen_center / half_plane_h;
	}

//
// Determine if object's radius is within view volume:
//
// point_in_view_space DOT plane_normal = distance inside volume
//
// TODO: MAKE THIS RETURN (outside, partial, inside) so we can use non-clipping pipeline
// when possible.
//
	virtual vis_state COMAPI object_visibility (const Vector &view_pos, float radius) const
	{
		vis_state result;

		if (view_pos.z > (radius - znear))
		{
		// Behind near plane.
			result = VS_NOT_VISIBLE;
		}
		else if ((view_pos.z + radius) < -znear)
		{
			result = VS_FULLY_VISIBLE;
		}
		else
		{
			result = VS_PARTIALLY_VISIBLE;
		}

	// If not fully near-clipped, check far plane.
		if (result != VS_NOT_VISIBLE)
		{
			if ((view_pos.z + radius) < -zfar)
			{
			// Beyond far plane.
				result = VS_NOT_VISIBLE;
			}
			else if (view_pos.z > (radius - zfar))
			{
				// Fully inside view plane; leave previous result alone.
			}
			else
			{
				result = VS_PARTIALLY_VISIBLE;
			}

		// If not fully clipped in z, check x.
			if (result != VS_NOT_VISIBLE)
			{
				float rx = (float) fabs(view_pos.x) * h_norm.x + view_pos.z * h_norm.z;
				// Effectively: dot_product(view_pos,h_norm);

				if (rx > radius)
				{
				// Outside view plane.
					result = VS_NOT_VISIBLE;
				}
				else if (rx < -radius)
				{
				// Fully inside view plane; leave previous result alone.
				}
				else
				{
				// Crossing plane.
					result = VS_PARTIALLY_VISIBLE;
				}

			// If not fully clipped in x, check y.
				if (result != VS_NOT_VISIBLE)
				{
					float ry = (float) fabs(view_pos.y) * v_norm.y + view_pos.z * v_norm.z;
					// Effectively: dot_product(view_pos,v_norm);
					if (ry > radius)
					{
					// Outside view plane.
						result = VS_NOT_VISIBLE;
					}
					else if (ry < -radius)
					{
					// Inside view plane; leave previous result alone.
					}
					else
					{
					// Crossing view plane.
						result = VS_PARTIALLY_VISIBLE;
					}
				}
			}
		}

		// check to see if it's smaller than 1 pixel
		if(result != VS_NOT_VISIBLE)
		{
			float radius_at_one = radius / -view_pos.z;

			if(PI * radius_at_one * radius_at_one * hpc * -vpc < 1.0f)
			{
				result = VS_SUB_PIXEL;
			}
		}

		return result;
	}

// Returns false if point is behind near plane (resulting screen coordinates invalid), true
// otherwise.
	virtual bool COMAPI point_to_screen(float & screen_x, float & screen_y, float & depth, const Vector & world_vector) const
	{
		bool result;

		Vector view_vector ( get_transform().inverse_rotate_translate(world_vector) );

		if (view_vector.z <= -znear)
		{
			float w = -1.0f / view_vector.z;
			screen_x = pane.x0 + x_screen_center + view_vector.x * w * hpc;
			screen_y = pane.y0 + y_screen_center + view_vector.y * w * vpc;
			depth = -view_vector.z;
			result = true;
		}
		else
		{
			result = false;
		}

		return result;
	}

//
// Returns relative vector from camera to point on near plane, expressed in WORLD coordinates.
//
	virtual void COMAPI screen_to_point (Vector & world_vector, float screen_x, float screen_y) const
	{
	// Scale point into perspective space.
		float sx = screen_x - (pane.x0 + x_screen_center);
		float sy = screen_y - (pane.y0 + y_screen_center);

		sx /= hpc;
		sy /= vpc;

		const Transform & cam2world = get_transform();
		world_vector = sx * cam2world.get_i() + sy * cam2world.get_j() - cam2world.get_k();
		world_vector *= znear;
	}
};
//------------------------------------------------------------------------------
//
struct BaseCamera : public DAComponent<__baseCameraImpl>, IEngineInstance
{
	INSTANCE_INDEX index;
	COMPTR<IEngine> engine;
	Transform transform;
	Vector velocity, ang_velocity;

	BaseCamera (IEngine *_engine, ViewRect *_pane)
	{   
		__baseCameraInit(_pane);

		ASSERT( _engine != NULL );

		index = INVALID_INSTANCE_INDEX;
		engine = _engine;

		// this archetype busines is just because it's no longer legal to call
		// create_instance() w/ an INVALID_ARCHETYPE_INDEX
		ARCHETYPE_INDEX arch = engine->allocate_archetype( NULL );
		index = engine->create_instance2(arch, this);
		engine->release_archetype( arch );
	}

	virtual ~BaseCamera (void)
	{
		if (index != INVALID_INSTANCE_INDEX)
			engine->destroy_instance(index);		
	}


	/* ICamera methods */
	
	virtual const Vector & COMAPI get_position (void) const
	{
		return transform.translation;
	}

	virtual const Transform & COMAPI get_transform (void) const
	{
		return transform;
	}

	virtual Transform COMAPI get_inverse_transform (void) const
	{
		// NOTE: This is the super-simple approach. If you want to cache the
		// inverse, overload this member in your own class.
		return transform.get_inverse();
	}


	/* IEngineInstance methods */
protected:
	virtual void   COMAPI initialize_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI create_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI destroy_instance (INSTANCE_INDEX index)
	{
	}
	virtual void   COMAPI set_position (INSTANCE_INDEX index, const Vector & position)
	{
		transform.set_position(position);
	}
	virtual const Vector & COMAPI get_position (INSTANCE_INDEX index) const
	{
		return transform.get_position();
	}
	virtual void   COMAPI set_orientation (INSTANCE_INDEX index, const Matrix & orientation)
	{
		transform.set_orientation(orientation);
	}
	virtual const Matrix & COMAPI get_orientation (INSTANCE_INDEX index) const
	{
		return transform.get_orientation();
	}
	virtual void      COMAPI set_transform (INSTANCE_INDEX index, const Transform & _transform)
	{
		transform = _transform;
	}
	virtual const Transform & COMAPI get_transform (INSTANCE_INDEX index) const
	{
		return transform;
	}
	virtual const Vector & COMAPI get_velocity (INSTANCE_INDEX object) const
	{
		return velocity;
	}
	virtual const Vector & COMAPI get_angular_velocity (INSTANCE_INDEX object) const
	{
		return ang_velocity;
	}

	virtual void COMAPI set_velocity (INSTANCE_INDEX object, const Vector & vel)
	{
		velocity = vel;
	}
	virtual void COMAPI set_angular_velocity (INSTANCE_INDEX object, const Vector & ang)
	{
		ang_velocity = ang;
	}

	virtual void COMAPI get_centered_radius (INSTANCE_INDEX object, float *radius, Vector *center) const
	{
		*radius = 0.0f;
		center->zero();
	}

	virtual void COMAPI set_centered_radius (INSTANCE_INDEX object, const float r, const Vector & center)
	{
	}

	/* BaseCamera methods */
public:
	virtual const Matrix & COMAPI get_orientation (void) const
	{
		return transform.get_orientation();
	}
	
	virtual void COMAPI set_position (const Vector & position)
	{
		transform.set_position(position);
	}

	virtual void COMAPI set_orientation (const Matrix & orientation)
	{
		transform.set_orientation(orientation);
	}

	virtual void COMAPI set_transform (const Transform & _transform)
	{
		transform = _transform;
	}

};

#endif
