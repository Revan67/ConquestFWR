//
//	data for one continuous LOD step
//	(shared by trimesh and exporter)

#ifndef TRI_LOD_H
#define TRI_LOD_H

#include "vector.h"

struct TriLODStep
{
	int removed_face_count; // number of faces removed
	int first_removed_face;

	int removed_batch_vertex_count; // number of vertices removed from batch_list 
	int morphed_vertex_chain_count; // number of vertex_chain indices that changed
	int first_vertex;				// index into a list of changed vertex_chain indices

	int vid1;						// physical vertex id 1
	int vid2;						// physical vertex id 2
	Vector v1;						// position of original vertex 1
	Vector v2;						// position of original vertex 2
	Vector v3;						// position of resulting vertex (NOT just (v1 + v2)/2)

	int removed_edge_count;			// number of edges removed
	int morphed_edge_count;			// number of edges that have changed
	int first_edge;					// first changed edge for this step in edge_list

	// UV stuff
	int morphed_uv_count;			// number of uv pairs affected by this step
	int first_morphed_uv;			// index of the first

	void Init(void)
	{
		removed_face_count = 0;
		first_removed_face = -1;
    
		removed_batch_vertex_count = 0;

		morphed_vertex_chain_count = 0;
		first_vertex = -1;

		vid1 = -1;
		vid2 = -1;
		v1.x = v1.y = v1.z =
		v2.x = v2.y = v2.z =
		v3.x = v3.y = v3.z = 0.0f;
    
		removed_edge_count = 0;
		morphed_edge_count = 0;
		first_edge = -1;

		first_morphed_uv = -1;
		morphed_uv_count = 0;
	}
};

#ifndef EXPORT_3DB // ignore the below for the exporters

class TriLODLib
{
public:
// static data
	int				step_cnt;				// number of edge collapses/vertex splits
	TriLODStep *	step_list;				// data for each split

	int				removed_face_cnt;		// number of removed faces
	int *			removed_face_list;		// group is'd of removed faces

	int				vertex_cnt;				// number of reindexed vertices
	int *			vertex_list;			// indices into object_vertex_chain

	int				uv_chain_count;			// length of uv_batch_chain
	int *			uv_batch_chain;			// chain of affected uv batch indices

	int				uv_count;				// length of uv lists
	int *			uv_high_id1;			// high detail indices into texture_vertex_list for above
	int *			uv_batch_cnt1;			// number of affected batch vertices for uv1
	int *			uv_batch_first1;		// position of first one in the chain
	int *			uv_high_id2;			
	int *			uv_batch_cnt2;
	int *			uv_batch_first2;
	int *			uv_low_id;				// low detail indices

	int				ref_cnt;				// allows sharing by multiple meshes

public:

	TriLODLib()
	{
		Init();
		AddRef();
	}

	~TriLODLib()
	{
		delete [] step_list;
		delete [] removed_face_list;
		delete [] vertex_list;
		delete [] uv_batch_chain;
		delete [] uv_high_id1;
		delete [] uv_batch_cnt1;
		delete [] uv_batch_first1;
		delete [] uv_high_id2;
		delete [] uv_batch_cnt2;
		delete [] uv_batch_first2;
		delete [] uv_low_id;
	}

	void Init(void)
	{
		memset(this, 0, sizeof(*this));
/*
		fraction =
		fraction_previous = 1.0f;
		threshold = 0.0001f;
*/
	}

	void Release(void)
	{
		ref_cnt--;
		if( ref_cnt < 1 )
		{
			delete this;
		}
	}

	void AddRef(void)
	{
		ref_cnt++;
	}
};

#endif

#endif
