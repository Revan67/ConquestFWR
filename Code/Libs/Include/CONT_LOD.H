//
//	data types to incrementaly modify an OpenFlame mesh -ms
//

#ifndef CONT_LOD_H
#define CONT_LOD_H

#include "vector.h"

struct lod_step
{
	int removed_face_count; // number of faces removed
	int first_removed_face;

	int morphed_face_count; // number of faces changed
	int first_morphed_face;	// index into list of face, normal & D coefficients 
							// for faces before modification

	int removed_batch_vertex_count; // number of vertices removed from batch_list 
	int morphed_vertex_chain_count; // number of vertex_chain indices that changed
	int first_vertex;				// index into a list of changed vertex_chain indices

	int vid1;						// physical vertex id 1
	int vid2;						// physical vertex id 2
	Vector v1;						// position of original vertex 1
	Vector v2;						// position of original vertex 2
	Vector v3;						// position of resulting vertex (NOT just (v1 + v2)/2)
	int v1_nid;						// normal id of original vertex 1
	float v1_D;						// D value of original vertex 1
	int v2_nid;						// normal id of original vertex 2
	float v2_D;						// D value of original vertex 2
	int v3_nid;						// normal id of resulting vertex
	float v3_D;						// D of resulting vertex

	int removed_edge_count;			// number of edges removed
	int morphed_edge_count;			// number of edges that have changed
	int first_edge;					// first changed edge for this step in edge_list

	void Init(void)
	{
		removed_face_count = 0;
		first_removed_face = -1;
    
		morphed_face_count = 0;
		first_morphed_face = -1;

		removed_batch_vertex_count = 0;

		morphed_vertex_chain_count = 0;
		first_vertex = -1;

		vid1 = -1;
		vid2 = -1;
		v1.x = v1.y = v1.z =
		v2.x = v2.y = v2.z =
		v3.x = v3.y = v3.z = 0.0f;
    
		v1_nid = -1;
		v1_D = 0.0f;
		v3_nid = -1;
		v3_D = 0.0f;

		removed_edge_count = 0;
		morphed_edge_count = 0;
		first_edge = -1;
	}
};

struct lod_uv_step
{
	int morphed_uv_count;			// number of uv pairs affected by this step
	int first_morphed_uv;			// index of the first

	void Init(void)
	{
		first_morphed_uv = -1;
		morphed_uv_count = 0;
	}
};

#endif
