//****************************************************************************
//*                                                                          *
//*  RENDERER.H: DA COM renderer access interface                            *
//*                                                                          *
//*  Source compatible with 32-bit 80386 C/C++                               *
//*                                                                          *
//*  V1.00 of 7-Jul-97: Initial                                              *
//*                                                                          *
//*  Author: John Miles                                                      *
//*                                                                          *
//****************************************************************************
//*                                                                          *
//*  Copyright (C) 1997 Digital Anvil, Inc.                                  *
//*                                                                          *
//****************************************************************************

#ifndef RENDERER_H
#define RENDERER_H

#ifndef US_TYPEDEFS
#include "typedefs.h"
#endif

#ifndef DACOM_H
#include "Dacom.h"      
#endif

#include "RenderProp.h"

//

typedef S32 ARCHETYPE_INDEX;
typedef S32 INSTANCE_INDEX;	// defined by Engine.h

//

class Vector;

// BBOX_*
//
// Defines the order of the values returned via get_archetype_bounding_box
// and get_instance_bounding_box.
//
const unsigned int BBOX_MAX_X = 0;
const unsigned int BBOX_MIN_X = 1;
const unsigned int BBOX_MAX_Y = 2;
const unsigned int BBOX_MIN_Y = 3;
const unsigned int BBOX_MAX_Z = 4;
const unsigned int BBOX_MIN_Z = 5;

//

// StatType
//
// Statistics available via IRenderer::get_*_statistics
//
enum StatType
{
	ST_NUM_PRIMITIVES,
	ST_NUM_LOD,
	ST_NUM_FACES,
	ST_NUM_VERTICES
} ;


// SA_*
//
// Flags accepted by split_archetype.
//
#define SA_KEEP_NONE		(1<<0)	// Do not keep primitives that intersect the plane.
#define SA_SPLIT_NONE		(1<<1)	// Do not split primitives that intersect the plane, instead
									// distribute the intersecting primitives to the two new archetypes 
									// in a random fashion (but still as a single primitive).
#define SA_SPLIT_JAGGED		(1<<2)	// 
									// 
									// 
#define SA_SPLIT_EXACT		(1<<3)	// Clip the primitives to the intersecting plane and distribute
									// each half to the appropriate new archetype.  Note that this
									// flag is mutually exclusive to SA_SPLIT_JAGGED.  Using both
									// will result in undefined behaviour.

//

// NOTE: Due to the macro nature of MAKE_IID, you cannot use another macro in place of the version
// number. Keep the second parameter in sync with the value of the explicit version macro, and increment
// both when the interface changes.
#define IRENDERER_VERSION 5
#define IID_IRenderer MAKE_IID("IRenderer",5)

// ......................................................................
//
// IRenderer
//
//
struct IRenderer : public IDAComponent
{

	// set_render_property
	//
	// Sets global rendering related properties.
	//
	// If no registered render component successfully responds to the property, GR_GENERIC is returned.
	// If at least one render component successfully responds to the property, GR_OK is returned.
	//
	virtual GENRESULT COMAPI set_render_property( const RenderProp name, struct DACOM_VARIANT value ) = 0;


	// get_render_property
	//
	// Gets the value of a global rendering property.
	//
	// If no registered render component successfully responds to the property, GR_GENERIC is returned.
	// If at least one render component successfully responds to the property, GR_OK is returned.
	//
	// The first component to return success causes this method to return immediately.
	//
	virtual GENRESULT COMAPI get_render_property( const RenderProp name, struct DACOM_VARIANT ou_value ) = 0;


	// get_archetype_statistics
	//
	// Returns information about a particular archetype.  Only the render component that 
	// created the specified archetype is queried for the statistics.
	//
	// arch_index can not be INVALID_ARCHETYPE_INDEX
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	// If the render component successfully responds to the property, out_value contains
	// the returned value of the statistic and TRUE is returned.
	//
	virtual BOOL32 COMAPI get_archetype_statistics( ARCHETYPE_INDEX arch_index, float lod_fraction, StatType statistic, DACOM_VARIANT out_value ) = 0;


	// get_archetype_bounding_box
	//
	// Returns the 3D bounding box a particular archetype.  Only the render component that 
	// created the specified archetype is queried for the bounding box.
	// 
	// arch_index can not be INVALID_ARCHETYPE_INDEX
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	// out_box can not be NULL.  Use the BBOX_MIN_foo and BBOX_MAX_foo values to determine
	// the ordering of the min and max values in the output.
	//
	// If this method succeeds, out_box contains the bounding box and TRUE is returned.
	//
	virtual BOOL32 COMAPI get_archetype_bounding_box( ARCHETYPE_INDEX arch_index, float lod_fraction, SINGLE *out_box ) = 0;

	// get_archetype_centroid
	//
	// Returns the centroid of the specifed level-of-detail of the specified archetype.
	// Only the render component that created the archetype is queried for the centroid.
	// Note that the 'centroid' in this context means the average of the vertices from
	// the origin in object space.
	//
	// arch_index can not be INVALID_ARCHETYPE_INDEX
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	// If this method succeeds, out_centroid contains the centroid and TRUE is returned.
	//
	virtual BOOL32 COMAPI get_archetype_centroid( ARCHETYPE_INDEX arch_index, float lod_fraction, Vector& out_centroid ) = 0;

	// split_archetype
	//
	// Split an archetype along the plane with specified normal distance d from the origin.
	// This splits all levels of detail, if any specific level of detail fails to split, the
	// new archetypes are not created.
	//
	// Returns TRUE if successful
	//
	// arch_index		Index of archetype to split
	// normal			Normal of split plane
	// d				Distance of plane from origin
	// r0,r1			Indices to store resulting archetypes in
	// sa_flags 		These flags determine how to handle particular situations that
	//					arise when splitting archetype data.  The value should be the
	//					logical OR of the SA_* flags above.
	// inst_index		If valid, face properties are copied to new archetypes		
	//
	virtual BOOL32 COMAPI split_archetype( ARCHETYPE_INDEX arch_index, const Vector& normal, float d,
										   ARCHETYPE_INDEX r0, ARCHETYPE_INDEX r1,
										   U32 sa_flags = SA_SPLIT_NONE,
										   INSTANCE_INDEX inst_index = INVALID_INSTANCE_INDEX ) = 0;



	
	// compute_lod_fraction
	//
	// Computes the recommended lod fraction based on the instance and a camera.  The
	// min and max render distances of the instance are scaled by distance_scale before
	// returning the value.  Note that this function may also take into account any
	// minimum and maximum rendering distances.
	//
	// inst_index can not be INVALID_INSTANCE_INDEX
	//
	// camera can not be NULL
	//
	// distance_scale can be any non-negative floating point value.
	//
	// Returns a negative value if inst_index is closer (in camera space) than the
	// minimum rendering distance or further than the maximum rendering distance.
	// Otherwise, returns an lod fraction between [0,1].
	//
	virtual float COMAPI compute_lod_fraction( INSTANCE_INDEX inst_index, struct ICamera *camera, float distance_scale ) = 0;

	// get_instance_projected_bounding_box
	//
	// Gets projected bounding box of the instance in screen coordinates and the depth 
	// of the center of the object in in camera space.
	//
	// inst_index can not be INVALID_INSTANCE_INDEX
	//
	// camera can not be NULL
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	// Returns FALSE if object is behind near plane or otherwise not visible to the camera.
	// Returns TRUE if the values in out_rect are valid (though not all are necessarily 
	// within screen bounds).
	//
	// Note this function replaces 'visible_rectangle' and has the same functionality.
	//
	virtual BOOL32 COMAPI get_instance_projected_bounding_box( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, struct ViewRect *out_rect, float& depth ) = 0;		

	// get_instance_projected_bounding_sphere
	//
	// Gets projected bounding sphere (center, radius) of the instance in screen coordinates
	// and the depth of the center of the object in in camera space.
	//
	// inst_index can not be INVALID_INSTANCE_INDEX
	//
	// camera can not be NULL
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	//
	// Returns FALSE if object is behind near plane or otherwise not visible to the camera.
	//
	virtual BOOL32 COMAPI get_instance_projected_bounding_sphere( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, float& out_center_x, float& out_center_y, float& out_radius, float& out_depth ) = 0;		

	// get_instance_projected_bounding_polygon
	// 
	// Returns vertices of polygon in screen space that surrounds the instance and 
	// its children. Resulting polygon could have as many as 6 + 3 * num_child_objects
	// vertices in the worst case. Upon success, the polygon vertices are stored
	// in out_vertices and the number of vertices in out_vertices is stored in 
	// out_num_verts. The vertices are sorted in clockwise order. The first vertex 
	// is the top-most left-most point, i.e. sorted by y, then x. The camera depth
	// of the center of the object is stored in out_depth.
	//
	// inst_index can not be INVALID_INSTANCE_INDEX 
	//
	// camera can not be NULL
	//
	// lod_fraction is in the range [0,1], zero means the lowest level of detail, one 
	// means the highest possible level of detail.  clients can use compute_lod_fraction()
	// to calculate this value.
	//
	// out_vertices can not be NULL.  An array of 64 elements should handle most common cases.
	//
	// max_num_vertices is the maximum number of vertices in the out_vertices array and 
	// can not be zero (0).
	//
	// Returns TRUE if vertex list is valid.
	// Returns FALSE otherwise (z-clipped, etc.).
	//
	// Note this function replaces 'visible_polygon' and has the same functionality.
	//
	virtual BOOL32 COMAPI get_instance_projected_bounding_polygon( INSTANCE_INDEX inst_index, struct ICamera *camera, float lod_fraction, int &out_num_vertices, struct ViewPoint *out_vertices, int max_num_vertices, float &out_depth ) = 0;


	// The following methods have a limited lifetime, use at your own peril.
	//
	virtual struct Mesh * COMAPI get_instance_mesh( INSTANCE_INDEX inst_index, const ICamera *camera = 0 ) = 0;
	virtual struct Mesh * COMAPI get_unique_instance_mesh(INSTANCE_INDEX inst_index, const ICamera * camera = 0 ) = 0;
	virtual GENRESULT COMAPI release_unique_instance_mesh (INSTANCE_INDEX inst_index ) = 0;
	virtual struct Mesh * COMAPI get_archetype_mesh( ARCHETYPE_INDEX arch_index, unsigned int level_of_detail = 0) = 0;
};

#endif	// EOF
