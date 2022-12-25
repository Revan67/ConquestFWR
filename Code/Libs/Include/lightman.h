#ifndef LIGHTMAN_H
#define LIGHTMAN_H
//--------------------------------------------------------------------------//
//                                                                          //
//                               lightman.h                                 //
//                                                                          //
//                  COPYRIGHT (C) 1998 BY DIGITAL ANVIL, INC.               //
//                                                                          //
//--------------------------------------------------------------------------//
/*
    $Header: /Conquest/Libs/Include/lightman.h 4     3/08/00 11:08a Rmarr $
*/			    
//--------------------------------------------------------------------------//

/*
	ILightManager interface. Provides a simple interface for managing lights 
	in the engine.

	NOTE: ALL RGB VALUES USED BY ILightManager functions are integer values in 
	the range 0 to 255. RGB values passed in by callers are clamped to this 
	range.

//---------------------------------------------
	int get_max_active_lights(void) const;

		RETURNS:
			Returns maximum active lights supported by renderer. 

//---------------------------------------------
	int COMAPI get_all_lights(ILight ** lights);

		OUTPUTS:
			lights		List of all "On" lights.

		RETURNS:
			Number of lights.

		NOTES:
			Pass NULL for the lights parameter to get the number of lights.

//---------------------------------------------
	int COMAPI get_active_lights(ILight ** lights);

		OUTPUTS:
			lights		List of currently active lights.

		RETURNS:
			Number of active lights.

		NOTES:
			Pass NULL for the lights parameter to get the number of active lights.

//---------------------------------------------
	int get_best_lights(ILight ** lights, 
						int num_lights, 
						const Vector & spot,
						float radius);

		INPUT:
			num_lights	Number of "best" lights requested. "lights" array must be at least this long.
			spot		Position of geometry to be lit.
			radius		Maximum distance considered for best lights.

		OUTPUT:
			lights		Array of best lights. Return value gives number of valid entries.

		RETURNS:
			Number of lights written to "lights" array. Always <= "num_lights" parameter.

		NOTES:
			Computes list of lights within specified radius of "spot" that will have the greatest effect 
			on geometry at "spot" as seen by the specified camera. The best light metric involves
			distance of light from spot and angle of incidence of light on spot as seen from camera.
			DOES NOT ACTIVATE OR DEACTIVATE ANY LIGHTS. User must call activate_lights().
			Only registered lights are considered by this method. (See register_light(), below.)
			Uses the camera specified in the last UpdateLighting() call.
			

//---------------------------------------------
	void COMAPI activate_lights(ILight * const * lights, int num_lights);

		INPUT:
			lights		List of lights to activate for rendering.
			num_lights	Length of list. Will be truncated if > get_max_active_lights().

		NOTES:
			Activates list of lights for rendering.

//---------------------------------------------
	void deactivate_lights(ILight * const * lights, int num_lights);

		INPUT:
			lights		List of lights to deactivate.
			num_lights	Length of list.


//---------------------------------------------
	void COMAPI deactivate_all_lights(void);

		NOTES:
			Deactivates all active lights.

//---------------------------------------------
	void COMAPI query_lights(bool * status, ILight * const * lights, int num_lights);	
		
		INPUT:
			lights		List of lights to query.
			num_lights	Length of 'lights' and 'status' arrays.

		OUTPUT:
			status		Active status of lights. Parallels "lights" array. For each light,
						this value is "true" if active, "false" if not.
			
//---------------------------------------------
	void COMAPI light_vertices (LightRGB * rgb, const Vector * vertices, const Vector * normals, int n, const Transform * world_to_object = 0);

		INPUT:
			vertices	List of vertex positions.
			normals		Parallel list of vertex normals.
			n			Number of vertices.
			world_to_object  Optional tranform to applied to lights.

		OUTPUT:
			rgb			Parallel list of RGB values for vertices.

		NOTES:
			If "world_to_object" is NULL, "vertices" and "normals" are assumed to be in WORLD coordinates. 
			If non-NULL, lights are transformed into object space before performing lighting calculations.

//---------------------------------------------
	void COMAPI set_ambient_light(int r, int g, int b);

		INPUT:
			r, g, b		Red, green, and blue color components of ambient light, 0-255.

		NOTES:
			Defaults to (0, 0, 0).

//---------------------------------------------

	void COMAPI get_ambient_light(int & r, int & g, int & b);

//---------------------------------------------
	void COMAPI register_light (ILight * light);
		INPUT:
			light: Instance of a light to consider for activation.
		OUTPUT:
			Adds light to an internal list of lights that may be considered in 
			a call to get_best_lights(). All registered lights are assumed to be "On".
			To turn off a light, call unregister_light().
		NOTES:
			The reference count of "light" is not affected by this call. Be sure to 
			unregister this light before destructing the instance.

//---------------------------------------------
	void COMAPI unregister_light (ILight * light);
		INPUT:
			light: Instance of a light to remove from the internal list.
		OUTPUT:
			Removes a light from an internal list of lights than may be considered in
			a call to get_best_lights(). It also removes the light from the active light list.
		NOTES:
			The reference count of "light" is not affected by this call.

//---------------------------------------------
	void COMAPI update_lighting (ICamera * camera);
		INPUT:
			camera: camera to use in future calls to get_best_lights().
		OUTPUT:
			Precalculates lighting values based on current state of all lights in the registered list.
		NOTES:
			Call this method at the top of the rendering loop. 
			(i.e. After all updating, but before first call to get_best_lights().)

*/

//

#ifndef DACOM_H
#include "DACOM.h"
#endif

typedef S32 INSTANCE_INDEX;     // defined by Engine.h
class Vector;
struct ILight;
class Transform;

typedef enum
{
	CF_DONT,				// don't clamp
	CF_INTENSITY,			// clamp intensity preserving color
	CF_COLOR				// clamp color preserving intensity 
} ClampFlags;

//
#ifndef LIGHTRGB_DEFINED
#define LIGHTRGB_DEFINED

struct LightRGB
{
	int r, g, b;

	LightRGB(void){}
	LightRGB(const int _r, const int _g, const int _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
};

#endif

struct LightRGB_U8
{
	U8 r, g, b;

	LightRGB_U8(void){}
	LightRGB_U8(const U8 _r, const U8 _g, const U8 _b)
	{
		r = _r;
		g = _g;
		b = _b;
	}
};

//

#define IID_ILightManager MAKE_IID("ILightManager",1)

// ................................................................................
//
//
//
//
struct ILightManager : public IDAComponent
{
	virtual void COMAPI set_ambient_light( int r, int g, int b ) = 0;
	virtual void COMAPI get_ambient_light( int & r, int & g, int & b ) = 0;

	virtual int	COMAPI get_max_active_lights( void ) const = 0;
	virtual void COMAPI activate_lights( ILight * const * lights, int num_lights ) = 0;
	virtual void COMAPI deactivate_lights( ILight * const * lights, int num_lights ) = 0;
	virtual void COMAPI deactivate_all_lights( void ) = 0;
	virtual int COMAPI get_active_lights( ILight ** lights ) = 0;

	virtual void COMAPI register_light( ILight * light ) = 0;
	virtual void COMAPI unregister_light( ILight * light ) = 0;
	virtual int COMAPI get_registered_lights( ILight ** lights ) = 0;

	virtual void COMAPI query_lights( bool * status, ILight * const * lights, int num_lights ) = 0;	
	virtual void COMAPI update_lighting( struct ICamera *camera ) = 0;

	virtual void COMAPI light_vertices( LightRGB * rgb, const Vector * vertices, const Vector * normals,
		int n, const Transform * world_to_object = 0, ClampFlags clamp = CF_INTENSITY ) = 0;

	virtual void COMAPI light_vertices_strided( LightRGB * rgb, const U32 lstride, const Vector * vertices,
		const U32 vstride, const Vector * normals, const U32 nstride, const U32 *nindices, const U32 nistride,
		int n, const Transform * world_to_object = 0, ClampFlags clamp = CF_INTENSITY, LightRGB * back_rgb = 0 ) = 0;

	virtual void COMAPI light_vertices_U8( LightRGB_U8 * rgb, const Vector * vertices, const Vector * normals,
		int n, const Transform * world_to_object = 0, ClampFlags clamp = CF_INTENSITY ) = 0;

};

#endif
