// IMesh.h
//
//
//

#ifndef __IMesh_h__
#define __IMesh_h__

//

#include "da_d3dtypes.h"
#include "IMaterial.h"

//

typedef U32 IM_ELEMENT;

const IM_ELEMENT IM_E_POINTS		= (1<<0);	// One Vector3 per element
const IM_ELEMENT IM_E_UV0S			= (1<<1);	// Two floats per element
const IM_ELEMENT IM_E_UV1S			= (1<<2);	// Two floats per element
const IM_ELEMENT IM_E_NORMALS		= (1<<3);	// One Vector3 per element
const IM_ELEMENT IM_E_COLOR0S		= (1<<4);	// One packed 32-bit BGRA per element
const IM_ELEMENT IM_E_COLOR1S		= (1<<5);	// One packed 32-bit BGRA per element
const IM_ELEMENT IM_E_FACE_NORMALS	= (1<<6);	// One Vector3 per element
const IM_ELEMENT IM_E_EDGES			= (1<<7);	// Two Vector3 per element
const IM_ELEMENT IM_E_EDGE_ANGLES	= (1<<8);	// One float per element

//

typedef U32 IM_OPTIMIZE_FLAG_BIT;

const IM_OPTIMIZE_FLAG_BIT IM_OF_STRIP	= (1<<0);	// convert to strips
const IM_OPTIMIZE_FLAG_BIT IM_OF_VCACHE	= (1<<1);	// optimize for vertex cache
const IM_OPTIMIZE_FLAG_BIT IM_OF_STATIC	= (1<<2);	// mark this mesh as static (suitable for caching in vidmem)

//

#define IID_IMesh MAKE_IID("IMesh", 1)

// 
// IMesh
//
// Use this interface to access mesh data.
//
// QUESTION:  This mesh is oriented towards large changes in the mesh.  i.e.
//            adding/removing/changing large amounts of vertex data.  Is this
//            a correct assumption?
//
struct IMesh : public IDAComponent
{
	// initialize
	//
	// Initialize the mesh to the default state.  The default state has
	// no facegroups in the mesh.
	//
	virtual GENRESULT COMAPI initialize( ) = 0;

	// verify()
	//
	// Check to see that the underlying mesh is still valid.  This will check that
	// there is a non-zero number of valid facegroups that are indexed correctly.
	//
	virtual GENRESULT COMAPI verify( void ) = 0;

	// optimize()
	//
	// Perform available optimizations on this mesh.  The bits passed in
	// control which operations *can* and *cannot* be performed.  However,
	// some implementations of this interface can choose not to implement
	// some optimizations at all.  
	//
	virtual GENRESULT COMAPI optimize( IM_OPTIMIZE_FLAG_BIT operations_mask ) = 0;

	// acquire_unique
	//
	// Disconnect this mesh from any underlying shared mesh.  This is
	// usually called before modifications are made to an instance.
	// If mesh is already unique nothing happens and GR_OK is returned.
	//
	// NOTE: if this mesh does not reference any shared mesh state,
	// NOTE: this method will return GR_GENERIC.  For instance, if
	// NOTE: the underlying mesh is an archetype, this method will 
	// NOTE: return failure.
	//
	virtual GENRESULT COMAPI acquire_unique( void ) = 0;

	// release_unique
	//
	// Reconnect this mesh to the original underlying shared mesh.
	//
	// NOTE: if this mesh does not reference any shared mesh state,
	// NOTE: this method will return GR_GENERIC.  For instance, if
	// NOTE: the underlying mesh is an archetype, this method will 
	// NOTE: return failure.
	//
	virtual GENRESULT COMAPI release_unique( void ) = 0;

	// get_num_facegroups()
	//
	// Retrieve the number of facegroups on this mesh.
	// 
	// Note that the only way to change the number of facegroups is to add
	// and/or remove facegroups with add_/remove_facegroup().
	//
	virtual GENRESULT COMAPI get_num_facegroups( U32 *out_num_facegroups ) = 0;

	// lock_facegroup
	//
	// Lock a facegroup for access.  This operation is required so that
	// the underlying mesh object can prepare for access to the particular facegroup.  
	//
	// Only one facegroup can be locked at a time.  If a facegroup is already locked,
	// this method will return GR_GENERIC.
	//
	// If the given facegroup number is not valid, this method returns GR_GENERIC.
	//
	// If this method returns anything but GR_OK, no facegroups are locked.
	//
	// This operation may be slow enough to warrant only using during 'load time'.
	//
	// If read_only is true the underlying mesh can remain in a more optimized state
	// plus read access will be faster.  any set_ methods will fail however
	//
	virtual GENRESULT COMAPI lock_facegroup( U32 facegroup_num, bool read_only ) = 0;

	// unlock_facegroup
	//
	// Unlock a facegroup previously locked via lock_facegroup().  This operation
	// is required so that the underlying mesh object can clean up any extra
	// data that is required only during facegroup access.  All data or indices
	// that are locked will also be unlocked by this call.
	//
	virtual GENRESULT COMAPI unlock_facegroup( void ) = 0;
	
	// add_facegroup()
	//
	// Add a facegroup to the mesh.  The facegroup is appended to the end
	// of the facegroups (therefore maintaining the current facegroup indexing).
	//
	// primitive_type is the type of primitive to use in the new facegroup.  If
	// the underlying implementation does not support the specified primtive type,
	// this method will return GR_GENERIC. No face groups can be locked.
	//
	virtual GENRESULT COMAPI add_facegroup( D3DPRIMITIVETYPE primitive_type ) = 0;

	// remove_facegroup()
	//
	// Remove the specified facegroup from the mesh.  If there are facegroups
	// after the specified facegroup, those facegroups are moved up one index
	// position.  
	//
	// No face groups (including the one being removed) can be locked. 
	//
	virtual GENRESULT COMAPI remove_facegroup( U32 facegroup_num ) = 0;
	
	// get_facegroup_primitive_type
	//
	// Retrieve the type of primitive in the currently locked facegroup.  
	//
	// Note that the only way to change the primitive type is to add a
	// new facegroup with the primitive type and copy set the data/index
	// for the new primitive type.  There is no automatic primitive conversion
	// supported via this interface.
	//
	virtual GENRESULT COMAPI get_facegroup_primitive_type( D3DPRIMITIVETYPE *out_primitive_type ) = 0;

	// get_facegroup_material
	//
	// Retrieve the material to be used when rendering the currently locked facegroup.  
	// If *out_material is not NULL, the interface will have been AddRef()'d.
	//
	// The material pointer **can** be NULL in which case the facegroup will **not** be
	// rendered.
	//
	virtual GENRESULT COMAPI get_facegroup_material( IMaterial **out_material ) = 0;

	// set_facegroup_material
	//
	// Change the material to be used when rendering the currently locked facegroup.
	// This will cause the mesh to release any reference to the current facegroup material.
	//
	// See the notes above in get_facegroup_material()
	//
	virtual GENRESULT COMAPI set_facegroup_material( IMaterial *material ) = 0;

	// get_facegroup_material_context
	//
	// out_extended_context is the application-specific context pointer that is passed
	// to the material when the locked facegroup is rendered with the given material.
	//
	virtual GENRESULT COMAPI get_facegroup_material_context( void **out_extended_context ) = 0;

	// set_facegroup_material_context
	//
	// Set the application-specific context pointer that is passed to the material 
	// when the locked facegroup is rendered with the given material.
	//
	virtual GENRESULT COMAPI set_facegroup_material_context( void *extended_context ) = 0;

	// lock_facegroup_data/unlock_facegroup_data
	//
	// These methods must be called to access the face/edge/vertex data associated
	// with the currently locked facegroup.  Any number of elements can be locked at 
	// once and the next unlock_data() call will unlock ALL of the previously locked 
	// data.  For instance:
	//
	// lock_facegroup_data( IM_E_POINTS | IM_E_NORMALS );
	// lock_facegroup_data( IM_E_EDGES );
	// unlock_facegroup_data();								// Unlocks points, normals, and 
	//							                            // edges data (but not indices).
	//
	// Calling lock_facegroup_data() attempting to lock an element that was previously 
	// locked will cause an error to be returned.  For example:
	//
	// lock_facegroup_data( IM_E_POINTS );
	// lock_facegroup_data( IM_E_POINTS );	// will return GR_GENERIC
	//
	// Calling unlock_facegroup_data() invalidates any pointers retrieved via 
	// get_facegroup_data()...
	//
	virtual GENRESULT COMAPI lock_facegroup_data( IM_ELEMENT element_mask ) = 0;
	virtual GENRESULT COMAPI unlock_facegroup_data( void ) = 0;

	// lock_facegroup_indices/unlock_facegroup_indices
	//
	// These methods must be called to access the face/edge/vertex indices associated
	// with the currently locked facegroup.  Any number of elements can be locked at 
	// once and the next unlock_indices() call will unlock ALL of the previously locked 
	// indices.  For instance:
	//
	// lock_facegroup_indices( IM_E_POINTS | IM_E_NORMALS );
	// lock_facegroup_indices( IM_E_EDGES );
	// unlock_facegroup_indices();							// Unlocks points, normals, and 
	//                                              // edges indices (but not data).
	//
	// Calling lock_facegroup_indices() attempting to lock an element that was previously locked
	// will cause an error to be returned.  For example:
	//
	// lock_facegroup_indices( IM_E_POINTS );
	// lock_facegroup_indices( IM_E_POINTS );	// will return GR_GENERIC
	//
	// Calling unlock_facegroup_indices() invalidates any pointers retrieved via 
	// get_facegroup_indices()...
	virtual GENRESULT COMAPI lock_facegroup_indices( IM_ELEMENT element_mask ) = 0;
	virtual GENRESULT COMAPI unlock_facegroup_indices( void ) = 0;

	// get_facegroup_data
	//
	// Retrieve information about the specified facegroup data for the currently locked
	// facegroup.  
	//
	// 'element_bit' can only specify a single vertex element.  If more than one bit
	// is set, the behavior is undefined.
	//
	// 'out_data' is a pointer to a const pointer to the requested data.
	// 'out_stride' is the number of bytes to advance a pointer to get from the 
	// beginning of an element to the beginning of the next element.	
	// 'out_count' returns the number of elements (not bytes) pointed to by '*out_data'.
	//
	// The type and size of the data pointed to by the out_data pointer is defined 
	// above in the IM_ELEMENT declaration.
	//
	// If the facegroup does not contain the specified information (i.e. no UV1 texture
	// coordinate set), this method will return GR_GENERIC.
	//
	virtual GENRESULT COMAPI get_facegroup_data( IM_ELEMENT element_bit, void const **out_data, U32 *out_stride, U32 *out_size, U32 *out_count ) = 0;

	// set_facegroup_data
	//
	// Set the information about the specified facegroup data for the currently locked
	// facegroup.
	//
	// The type and size of the data pointed to by the data pointer is defined 
	// above in the IM_ELEMENT declaration.
	//
	virtual GENRESULT COMAPI set_facegroup_data( IM_ELEMENT element_bit, const void *data, U32 stride, U32 size, U32 count ) = 0;

	// get_facegroup_indices
	//
	// Retrieve the indices for the specified facegroup indices for the currently
	// locked facegroup.
	//
	// QUESTION: How do we explain indexing for the different types of elements?
	//
	virtual GENRESULT COMAPI get_facegroup_indices( IM_ELEMENT element_bit, U16 const **out_indices, U32 *out_count ) = 0;
	
	// set_facegroup_indices
	//
	// Set the indices for the specified facegroup indices for the currently locked
	// facegroup.
	//
	// If 'count' is not valid for the primitive type of the currently locked facegroup,
	// this method returns GR_GENERIC.
	//
	virtual GENRESULT COMAPI set_facegroup_indices( IM_ELEMENT element_bit, const U16 *indices, U32 count ) = 0;

	// get_facegroup_indices_count
	//
	// Retreive the number of indices associated with the currently locked
	// facegroup. 
	// 
	// If there is no currently locked facegroup or the current facegroup is
	// invalid for any reason, GR_GENERIC is returned and out_num_indices is
	// set to 0.
	//
	// Otherwise, the number of indices in the current facegroup is returned.
	//
	// Note that this method can return zero (0) indices and return GR_OK *or*
	// GR_GENERIC depending on the validity of the facegroup.
	//
	// Note that there is no support for determining metadata about the different
	// primitive types supported via this interface.  It is up to the client
	// to make sure that it can index correctly based on the primitive type.
	//
	virtual GENRESULT COMAPI get_facegroup_indices_count( U32 *out_num_indices ) = 0;

	// set_facegroup_indices_count
	//
	// Sets the number of indices in the currently locked facegroup.
	//
	// If there is no locked face group, or the group is locked read_only,
	// or any face group elements are currenty locked GR_GENERIC is returned.
	//
	// If the previous number of indices is non-zero, then this method
	// will clear any currently set face/vertex/edge indices (not data) and
	// invalidate the currently locked facegroup.
	//
	virtual GENRESULT COMAPI set_facegroup_indices_count( U32 num_indices ) = 0;

	// add_element
	//
	// adds the specified elements to the currently locked face group.
	// no face group indices can be locked or this will fail
	// if elements already exist this will still return GR_OK
	virtual GENRESULT COMAPI add_elements( IM_ELEMENT element_bits ) = 0;

	// remove_element
	//
	// removes the specified elements from the currently locked face group.
	// no face group indices can be locked or this will fail
	// if elements don't exist this will still return GR_OK
	virtual GENRESULT COMAPI remove_elements( IM_ELEMENT element_bits ) = 0;
};

//

#endif	// EOF
