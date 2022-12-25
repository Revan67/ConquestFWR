#ifndef BASEMESH_H
#define BASEMESH_H

//

#include "3dmath.h"

//
// BaseMesh is a substruct that contains just the geometric description of a mesh,
// i.e. no material or texture data is present.
//
struct BaseMesh
{
//
// List of unique plane orientations (surface normals) for all faces.
//
// Surface normal (plane) equation: Ax+By+Cz-D=0, where A, B, and C
// are coefficients that uniquely specify a plane's orientation, and D
// specifies the position of the plane in object space (i.e., its position
// along its infinite perpendicular axis)
// 
// All parallel faces in the mesh share the same A, B, and C coefficients, 
// but only coplanar faces share the same D coefficient. This property 
// is useful for rapid backface culling and BSP traversal, as it allows us
// to re-use dot product calculations between all faces which are either
// coplanar or parallel to each other.
//
	S32        normal_cnt;              // # of unique surface normals 
	Vector    *normal_ABC;              // Normal A, B, and C coefficients (Ax+By+Cz)


//
// List of unique 3D object-space vertices in mesh
//
	S32        object_vertex_cnt;       // # of unique object-space vertices
	Vector    *object_vertex_list;      

// Indices into normal list
	S32* vertex_normal;		   // In same order as object_vertex_list.
	SINGLE* vertex_D_coefficient;		

//
// List of object-space and texture-space vertex chains for faces
// These arrays are lists of indices to successive vertices in each face
//
	S32     vertex_reference_cnt;       // # of vertex references

	S32    *object_vertex_chain;        // Ordered lists of vertex references for each face.

//
// List of faces in mesh, defined as a closed list of vertices in object
// (XYZ) and texture (UV) space, with an associated surface normal 
// (plane orientation in object space) and D-coefficient (plane offset 
// along its normal axis)
//
	S32     face_cnt;                   // # of faces

	S32    *face_num_vertices;          // # of vertices in face
	S32    *face_vertices;              // Offset in obj/txt vertex chains

	S32    *face_normal;                // Indices into surface normal list
	SINGLE *face_D_coefficient;         // D-coefficients of normals
};

//

#endif
