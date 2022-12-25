// IMaterial
//
//
//

#ifndef __IMaterial_h__
#define __IMaterial_h__

//

#include "Engine.h"
#include "VertexBufferDesc.h"
#include "IRenderPrimitive.h"
#include "rendpipeline.h"

//

const U32 IM_MAX_NAME_LEN = 64;
const U32 IM_MAX_TYPE_LEN = 64;


//
// MaterialContext
//
//

struct MaterialContext
{
	INSTANCE_INDEX inst_index;				// Instance index of the object being rendered.  This *can* be
											// INVALID_INSTANCE_INDEX if there is no associated index.

	const Transform *object_to_world;		// Object transform plus any user-added transform (including scale)

	const Transform *world_to_view;			// Camera world to view transform
	
	const Transform *object_to_view;		// Complete transform (and current modelview matrix)

	IRP_VERTEXBUFFERHANDLE vertex_buffer;	// If this is not IRP_INVALID_VB_HANDLE, this contains the cached
											// version of the archetype vertices.  This vertex buffer is assumed
											// to be read-only.  If the material does not need to do anything
											// special with the vertex values AND it does not "append" any 
											// new vertex items, it can use this to render instead of acquiring
											// a vertex buffer, copying the data, and calling render_with_vb.

	const void  *extended_context;			// Application specific extended context associated with the
											// facegroup being rendered via this material.
};

//

// Render Flags
//
// Used in calls to IMaterial::render()
//
#define IM_RF_PERSISTENT_DATA	(1<<0)	// Tells the material that the vertex and index data
										// will be valid beyond the return of the render call.
#define IM_RF_DONT_CLIP			(1<<1)	// Tells the material not to clip the data.  (can be ignored)

//

//
// IMaterial
//
//
#define IID_IMaterial MAKE_IID("IMaterial",1)
//
struct IMaterial : public IDAComponent
{
	// initialize
	//
	// This method does any initialization required for the lifetime
	// of the material.
	//
	virtual GENRESULT COMAPI initialize( IDAComponent *system_container ) = 0;

	// load_from_filesystem
	//
	// Load bindings for material parameters from the given filesystem.
	//
	// This method does *not* have to attach to resources (textures, etc..)
	// and should *not* fail if texture references cannot be acquired.
	// The verify method below takes care of this...
	//
	virtual GENRESULT COMAPI load_from_filesystem( IFileSystem *IFS ) = 0;

	// verify
	//
	// Verify that the material will work on the current hardware.
	//
	// This method should choose the number of passes etc. at this
	// point and release any unnecessary references.
	//
	// Note that materials should "connect" to necessary textures
	// during this method and *not* during the load_from_filesystem()
	// call.
	//
	virtual GENRESULT COMAPI verify( U32 max_num_passes, float max_detail_level ) = 0;

	// update
	//
	// Update this material's current state.  This method updates animated
	// textures, animated material values, etc...
	//
	virtual GENRESULT COMAPI update( float dt ) = 0;

	// apply
	//
	// Apply the renderstate necessary to render with this material.
	//
	virtual GENRESULT COMAPI apply( void ) = 0;

	// clone
	//
	// Make a duplicate of this material.  
	//
	virtual GENRESULT COMAPI clone( IMaterial **out_Material ) = 0;

	// render
	//
	// Render the given primitive and vertex data with this material.
	//
	virtual GENRESULT COMAPI render( MaterialContext *context,
									 D3DPRIMITIVETYPE PrimitiveType,
									 VertexBufferDesc *Vertices, 
									 U32 StartVertex, 
									 U32 NumVertices,
									 U16 *FaceIndices,
									 U32 NumFaceIndices,
									 U32 im_rf_flags ) = 0;

	// set_name
	//
	// Set the name of this material.
	//
	// NOTE: Changing the name without updating any underlying MaterialLibrary
	// NOTE: could be dangerous.  If one changes the name of a material, one 
	// NOTE: should also remove the material under the old name and add it under
	// NOTE: the new name.
	//
	virtual GENRESULT COMAPI set_name( const char *new_name ) = 0;

	// get_name
	//
	// Get the name of this material.
	//
	virtual GENRESULT COMAPI get_name( char *out_name, U32 max_name_len ) = 0;

	// get_type
	//
	// Get the string that describes this material type.
	//
	virtual GENRESULT COMAPI get_type( char *out_type, U32 max_type_len ) = 0;

	// get_num_passes
	//
	// Retrieve the number of passes this material uses.
	//
	virtual GENRESULT COMAPI get_num_passes( U32 *out_num_passes ) = 0;

};


#endif // EOF

