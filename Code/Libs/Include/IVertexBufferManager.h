// IVertexBufferManager
//
//
//

#ifndef __IVertexBufferManager_h__
#define __IVertexBufferManager_h__

//

#include "rendpipeline.h"
#include "VertexBufferDesc.h"

//

#define IVBM_AVBF_EXACT		(1<<0)	// Find exact match to specified vertex buffer
#define IVBM_AVBF_NO_CREATE	(1<<1)	// Do not create a new vertex buffer if non found.

//

#define IID_IVertexBufferManager MAKE_IID("IVertexBufferManager",1)

//

//
// IVertexBufferManager
//
//
struct IVertexBufferManager : public IDAComponent
{
	// initialize
	//
	// profile_name specifies the section in the profile (.ini) to use
	// to initialize the vertex buffer manager.  If profile_name is NULL,
	// the default profile is used.  The name of the default profile is
	// implementation dependent and does not necessarily read any values
	// from the profile.
	//
	// Returns GR_OK if the manager is successfully inititalized.
	// Returns failure otherwise.
	// 
	virtual GENRESULT COMAPI initialize( const char *profile_name ) = 0;


	// cleanup
	//
	// Cleanup the managed vertex buffers.
	//
	virtual GENRESULT COMAPI cleanup( void ) = 0;


	// add_vertex_buffer
	//
	// Adds a managed vertex buffer.
	//
	// NOTE: irp_vbf_flags is a collection of IRenderPipeline create_vertex_buffer()
	// flags.  
	//
	// Returns GR_OK if the vertex buffer is added and managed.
	// Returns failure otherwise.
	//
	virtual GENRESULT COMAPI add_vertex_buffer( U32 vertex_format, U32 num_verts, U32 irp_vbf_flags ) = 0;


	// remove_all_vertex_buffers
	//
	// Removes (and destroys) all managed vertex buffers.
	//
	// Returns GR_OK if all of the vertex buffers are removed.
	//
	virtual GENRESULT COMAPI remove_all_vertex_buffers( void ) = 0;
	

	// acquire_vertex_buffer
	//
	// num_verts is the minimum number of vertices that is needed in the buffer.
	// If there is no vertex buffer with at least num_vertices vertices and the
	// IVBM_AVBF_NO_CREATE flags is set, this method returns an error.  Otherwise,
	// a vertex buffer that is compatible with the requested vertex buffer is returned.
	//
	// vertex_format is the type of vertex required.  If there is no vertex buffer
	// with the specified vertex type and the IVBM_AVBF_EXACT flag is set, this method
	// will return an error.  Otherwise, this method will find a vertex format that
	// is compatible (contains all of the requested elements) and return that vertex
	// buffer.
	//
	// irp_vbf_flags defines what type of vertex buffer you would like.  
	//
	// TODO: document the return values
	//
	// If this method returns anything but GR_OK, the values in out_vbhandle,
	// out_vbmemptr, out_numvertices, and out_vertexformat are unchanged.
	//
	virtual GENRESULT COMAPI acquire_vertex_buffer( U32 vertex_format, 
													U32 num_verts, 
													U32 irp_vbf_flags, 
													U32 irp_lock_flags, 
													U32 ivbm_avbf_flags, 
													IRP_VERTEXBUFFERHANDLE *out_vbhandle,
												    void **out_vbmemptr, 
													U32 *out_vertex_format,
													U32 *out_num_verts ) = 0;


	// copy_vertex_data
	//
	// Fill in the given vertex buffer with the data in described by the 
	// vertex data described in the given VertexBufferDesc.
	//
	virtual GENRESULT COMAPI copy_vertex_data( void *dst_buffer, U32 dst_vertex_format, VertexBufferDesc *src_buffer ) = 0;


	// release_vertex_buffer
	//
	// Release a vertex buffer that was previously acquired via acquire_vertex_buffer().
	// This makes the buffer available to future acquire_vertex_buffer() calls.
	//
	virtual GENRESULT COMAPI release_vertex_buffer( IRP_VERTEXBUFFERHANDLE vbhandle ) = 0;
};


#endif // EOF

